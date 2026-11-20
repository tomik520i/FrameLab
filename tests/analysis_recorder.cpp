#include "AnalysisRecorder.hpp"
#include <iostream>
void require(bool x,const char* text){if(!x)throw std::runtime_error(text);}
int main(){try{
 auto root=std::filesystem::temp_directory_path()/("FrameLab-recorder-test-"+std::to_string(GetCurrentProcessId()));
 std::filesystem::create_directories(root);AnalysisRecorder rec;
 try{rec.start(root,"Synthetic recorder test",0,0,1,4,true,false,true,0,"Test",true,60,40);throw std::runtime_error("Invalid preflight accepted");}catch(const std::runtime_error&){require(!std::filesystem::exists(root/"recordings"),"Failed preflight created a take");}
 constexpr unsigned w=64,h=36;const size_t bytes=w*h*4;rec.start(root,"Synthetic recorder test",w,h,1,4,true,false,true,0,"Test",true,60,60);
 double start=rec.startTime;std::vector<unsigned char> frame(bytes);std::vector<std::vector<unsigned char>> originals;
 for(unsigned n=0;n<900;++n){for(size_t i=0;i<bytes;++i)frame[i]=i%4==3?255:static_cast<unsigned char>((i*3+n*11)%256);originals.push_back(frame);double t=start+double(n)/60+.0001;rec.raw(frame.data(),w,h,n,t);
  for(unsigned k=1;k<4;++k){AnalysisRecorder::Item item;item.kind='G';item.id=n*3+k;item.a=n? n-1:0;item.b=n;item.w=w;item.h=h;item.t=k/4.;item.time=t+k*.001;item.pixels=frame;rec.push(std::move(item));}}
 rec.presented(false,899,899,899,0);rec.confirmed(true,2700,1001,100,100,12345);rec.confirmed(false,899,1002,101,101,12346);
 rec.finish("Synthetic recorder verification");rec.worker.join();require(rec.rawCount==900&&rec.genCount==2700&&rec.dropped==0,"4x frame counts / drops mismatch");require(rec.presentCount==1,"Display confirmations inflated submission count");
 require(!std::filesystem::exists(rec.folder/"raw.bgra")&&!std::filesystem::exists(rec.folder/"generated.bgra"),"Uncompressed disk scratch was created");
 std::ifstream raw(rec.folder/"R.flp",std::ios::binary),gen(rec.folder/"G.flp",std::ios::binary);std::vector<unsigned char> got;
 auto decode=[&](std::ifstream& file){uint32_t header[5]{};file.read((char*)header,sizeof(header));require(header[0]==0x31504c46&&header[1]==bytes,"Invalid FLP header");auto block=std::make_shared<LosslessRamBuffer::Block>();block->rawBytes=header[1];block->bytes.resize(header[2]);block->compressed=header[3]&1;block->rgbDelta=header[3]&2;block->alpha=header[4];file.read((char*)block->bytes.data(),header[2]);require(bool(file),"Truncated archive");LosslessRamBuffer::unpack(block,got);};
 for(const auto& expected:originals){decode(raw);require(got==expected,"Raw pixels changed on disk");for(int k=0;k<3;++k){decode(gen);require(got==expected,"Generated pixels changed on disk");}}
 require(std::filesystem::exists(rec.folder/"result.json"),"Missing result metadata");std::ifstream confirmations(rec.folder/"display-confirmations.csv");std::string line;int lines=0;while(std::getline(confirmations,line))++lines;require(lines==3,"Missing display confirmation rows");std::cout<<"4x recorder at 64x36: 900 RAW + 2700 generated, every output byte verified, zero drops; failed preflight creates no folder; display confirmations persisted\n";
 std::cout<<"Synthetic verification artifacts: "<<root.string()<<'\n';return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
