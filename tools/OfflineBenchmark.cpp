#include "Graphics.hpp"
#include "Vfg.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <iomanip>

using Clock=std::chrono::steady_clock;
double ms(Clock::time_point a){return std::chrono::duration<double,std::milli>(Clock::now()-a).count();}
struct Pair {uint64_t id,a,b,aid,bid;int64_t truth[3];double ta,tb;};
double avg(const std::vector<double>& v){double total=0;for(auto x:v)total+=x;return v.empty()?0:total/v.size();}
double p95(std::vector<double> v){std::sort(v.begin(),v.end());return v.empty()?0:v[std::min(v.size()-1,size_t(std::ceil(v.size()*.95))-1)];}
int wmain(int argc,wchar_t** argv){try{
 std::map<std::wstring,std::wstring> options;
 for(int i=1;i<argc;++i){std::wstring key=argv[i];if(key==L"--save-all")options[key]=L"1";else{if(i+1==argc)throw std::runtime_error("Missing option value");options[key]=argv[++i];}}
 auto required=[&](const wchar_t* key){if(!options.count(key))throw std::runtime_error("Missing required argument");return options[key];};
 const auto inputPath=std::filesystem::path(required(L"--input")),manifest=std::filesystem::path(required(L"--pairs")),out=std::filesystem::path(required(L"--out")),runtime=std::filesystem::path(required(L"--runtime"));
 unsigned w=std::stoul(required(L"--width")),h=std::stoul(required(L"--height")),quality=std::stoul(required(L"--quality")),multiplier=std::stoul(required(L"--multiplier"));
 bool cuts=std::stoul(required(L"--cuts"))!=0,saveAll=options.count(L"--save-all")!=0;
 if(!w||!h||quality>2||multiplier<2||multiplier>4)throw std::runtime_error("Invalid dimensions/mode");
 if(std::filesystem::exists(out))throw std::runtime_error("Output exists; choose a new run name");
 size_t bytes=size_t(w)*h*4;std::ifstream csv(manifest);if(!csv)throw std::runtime_error("Cannot open pair manifest");
 std::string line;std::getline(csv,line);std::vector<Pair> pairs;
 while(std::getline(csv,line)){if(line.empty())continue;std::replace(line.begin(),line.end(),',',' ');std::istringstream row(line);Pair p{};
  if(!(row>>p.id>>p.a>>p.b>>p.truth[0]>>p.truth[1]>>p.truth[2]>>p.aid>>p.bid>>p.ta>>p.tb))throw std::runtime_error("Bad pair manifest");pairs.push_back(p);}
 if(pairs.empty())throw std::runtime_error("Empty benchmark");
 // Preload once. Disk reads, metrics, saving and readback are outside the timing pass.
 std::map<uint64_t,std::vector<unsigned char>> pixels;std::ifstream input(inputPath,std::ios::binary);if(!input)throw std::runtime_error("Cannot read pixels");
 auto load=[&](uint64_t offset){if(pixels.count(offset))return;auto& v=pixels[offset];v.resize(bytes);input.clear();input.seekg(offset);if(!input.read((char*)v.data(),bytes))throw std::runtime_error("Truncated source frame");};
 for(auto& p:pairs){load(p.a);load(p.b);for(unsigned i=0;i<multiplier-1;++i)if(p.truth[i]>=0)load(uint64_t(p.truth[i]));}
 Graphics g;g.initDevice();Texture output;output.create(g.device.Get(),w,h);Texture copies[3];for(auto& t:copies)t.create(g.device.Get(),w,h);
 D3D11_TEXTURE2D_DESC desc{};output.texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
 ComPtr<ID3D11Texture2D> staging;check(g.device->CreateTexture2D(&desc,nullptr,&staging),"Benchmark staging");
 Vfg vfg;vfg.initialize(g.device.Get(),output.texture.Get(),w,h,quality,runtime,cuts);
 auto wait=[&]{auto begin=Clock::now();while(!vfg.ready()){if(ms(begin)>30000)throw std::runtime_error("GPU timeout");SwitchToThread();}};
 auto upload=[&](const Pair& p){vfg.upload(pixels[p.a].data(),0);vfg.upload(pixels[p.b].data(),1);};
 for(unsigned n=0;n<4;++n){upload(pairs[n%pairs.size()]);for(unsigned i=1;i<multiplier;++i){vfg.generate(1,false,i,multiplier);wait();g.context->CopyResource(copies[i-1].texture.Get(),output.texture.Get());}}
 std::filesystem::create_directories(out);
 std::ofstream timing(out/L"timing.csv");timing<<"pair_id,index,upload_ms,host_submit_ms,ready_wall_ms,gpu_ms,pair_ready_ms\n"<<std::setprecision(10);
 std::vector<double> pairTimes,uploadTimes,hostTimes,readyTimes,gpuTimes;std::vector<double> phaseGpu[3];
 for(auto& p:pairs){auto begin=Clock::now();upload(p);double uploadMs=ms(begin);uploadTimes.push_back(uploadMs);
  for(unsigned i=1;i<multiplier;++i){auto run=Clock::now();vfg.generate(1,false,i,multiplier);double host=ms(run);wait();double ready=ms(run);double gpu=vfg.gpuMs();
   g.context->CopyResource(copies[i-1].texture.Get(),output.texture.Get());
   double pair=ms(begin);timing<<p.id<<','<<i<<','<<uploadMs<<','<<host<<','<<ready<<','<<gpu<<','<<pair<<'\n';
   hostTimes.push_back(host);readyTimes.push_back(ready);gpuTimes.push_back(gpu);phaseGpu[i-1].push_back(gpu);if(i==multiplier-1)pairTimes.push_back(pair);
  }
 }
 timing.close();
 std::ofstream rows(out/L"frames.csv");rows<<"pair_id,index,source_A,source_B,source_gap_ms,psnr_db,hold_psnr_db,blend_psnr_db,exact_A,exact_B,rgb_fnv64,byte_offset\n"<<std::setprecision(10);
 std::ofstream generated;if(saveAll)generated.open(out/L"generated.bgra",std::ios::binary);
 std::vector<unsigned char> actual(bytes);std::vector<double> psnrs,holds,blends;size_t count=0,copiesCount=0;
 auto psnr=[&](double sum){return sum==0?100.:10*std::log10(255.*255.*double(w)*h*3/sum);};
 auto bmp=[&](std::wstring name){BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(ih);fh.bfSize=DWORD(fh.bfOffBits+bytes);ih.biSize=sizeof(ih);ih.biWidth=w;ih.biHeight=-LONG(h);ih.biPlanes=1;ih.biBitCount=32;std::ofstream f(out/name,std::ios::binary);f.write((char*)&fh,sizeof(fh));f.write((char*)&ih,sizeof(ih));f.write((char*)actual.data(),bytes);if(!f)throw std::runtime_error("Image save failed");};
 for(size_t n=0;n<pairs.size();++n){auto& p=pairs[n];upload(p);auto& a=pixels[p.a];auto& b=pixels[p.b];
  for(unsigned i=1;i<multiplier;++i){vfg.generate(1,false,i,multiplier);wait();g.context->CopyResource(staging.Get(),output.texture.Get());D3D11_MAPPED_SUBRESOURCE map{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"Readback");
   for(unsigned y=0;y<h;++y)memcpy(actual.data()+size_t(y)*w*4,(char*)map.pData+size_t(y)*map.RowPitch,w*4);g.context->Unmap(staging.Get(),0);
   double sum=0,hold=0,blend=0,t=double(i)/multiplier;bool eqA=true,eqB=true;uint64_t hash=14695981039346656037ull;
   const auto* truth=p.truth[i-1]>=0?pixels[uint64_t(p.truth[i-1])].data():nullptr;
   for(size_t z=0;z<bytes;z+=4)for(unsigned c=0;c<3;++c){unsigned char v=actual[z+c];hash=(hash^v)*1099511628211ull;eqA&=v==a[z+c];eqB&=v==b[z+c];
    if(truth){double e=double(v)-truth[z+c],ha=double(a[z+c])-truth[z+c],mix=a[z+c]*(1-t)+b[z+c]*t-truth[z+c];sum+=e*e;hold+=ha*ha;blend+=mix*mix;}}
   double q=-1,hq=-1,bq=-1;if(truth){q=psnr(sum);hq=psnr(hold);bq=psnr(blend);psnrs.push_back(q);holds.push_back(hq);blends.push_back(bq);}
   rows<<p.id<<','<<i<<','<<p.aid<<','<<p.bid<<','<<(p.tb-p.ta)*1000<<','<<q<<','<<hq<<','<<bq<<','<<eqA<<','<<eqB<<','<<hash<<','<<count*bytes<<'\n';copiesCount+=eqA||eqB;
   if(saveAll)generated.write((char*)actual.data(),bytes);
   if(n==0||n==pairs.size()/2||n+1==pairs.size())bmp(L"pair-"+std::to_wstring(p.id)+L"-phase-"+std::to_wstring(i)+L".bmp");++count;
  }
 }
 if(saveAll&&!generated)throw std::runtime_error("Pixel output failed");
#ifdef FRAMELAB_VFG_REBIND_EACH_OUTPUT
 const char* binding="legacy-per-output";
#else
 const char* binding="once-per-pair";
#endif
 std::ofstream summary(out/L"summary.json");summary<<std::setprecision(10)<<"{\"binding\":\""<<binding<<"\",\"width\":"<<w<<",\"height\":"<<h<<",\"quality\":"<<quality<<",\"multiplier\":"<<multiplier<<",\"automatic_cuts\":"<<(cuts?"true":"false")<<",\"pairs\":"<<pairs.size()<<",\"outputs\":"<<count<<",\"endpoint_copies\":"<<copiesCount<<",\"truth_frames\":"<<psnrs.size()<<",\"psnr_mean_db\":"<<avg(psnrs)<<",\"hold_psnr_mean_db\":"<<avg(holds)<<",\"blend_psnr_mean_db\":"<<avg(blends)<<",\"upload_mean_ms\":"<<avg(uploadTimes)<<",\"host_submit_mean_ms\":"<<avg(hostTimes)<<",\"ready_wall_mean_ms\":"<<avg(readyTimes)<<",\"gpu_mean_ms\":"<<avg(gpuTimes)<<",\"gpu_p95_ms\":"<<p95(gpuTimes)<<",\"pair_ready_mean_ms\":"<<avg(pairTimes)<<",\"pair_ready_p95_ms\":"<<p95(pairTimes)<<",\"phase_gpu_mean_ms\":[";
 for(unsigned i=0;i<multiplier-1;++i){if(i)summary<<',';summary<<avg(phaseGpu[i]);}summary<<"],\"limits\":\"Offline, preloaded identical input pairs; four warmup pairs excluded. Timing pass excludes disk, readback, metrics, presentation and capture/source latency. Quality pass uses full RGB pixels; no ground-truth quality score without withheld frames.\"}\n";
 std::cout<<"Completed "<<pairs.size()<<" pairs / "<<count<<" outputs, "<<binding<<", pair ready "<<avg(pairTimes)<<" ms, PSNR "<<avg(psnrs)<<"\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
