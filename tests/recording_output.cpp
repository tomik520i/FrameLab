#include "RecordingOutput.hpp"
#include "AnalysisRecorder.hpp"
#include <iostream>
#include <thread>
int wmain(int argc,wchar_t** argv){try{
 if(argc!=2)throw std::runtime_error("Expected test output directory");
 std::filesystem::path root=argv[1];std::filesystem::create_directories(root);
 constexpr unsigned w=640,h=360;Graphics g;g.initDevice();Texture source,enhanced;source.create(g.device.Get(),w,h);enhanced.create(g.device.Get(),w,h);
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={float(w),float(h)};io.DeltaTime=1.f/60;ui::initialize();ImGui_ImplDX11_Init(g.device.Get(),g.context.Get());
 RecordingOutput output;AnalysisRecorder rec;rec.pipelineSettings="{\"test\":true,\"nr_strength\":0.5,\"subtitles\":true}";rec.start(root,"Synthetic compositor (not live hardware benchmark)",w,h,1,2,true,false,true,0,"Test",false,60,60);
 SubtitleSettings settings;settings.enabled=true;settings.fontPercent=5.5f;settings.position=.92f;settings.horizontal=.5f;settings.placement=0;settings.verticalAlign=2;settings.sanitize();settings.save(rec.folder/"subtitle-settings.txt");
 std::vector<unsigned char> a(w*h*4),b(w*h*4);
 for(unsigned i=0;i<90;++i){
  for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){auto p=(y*w+x)*4;a[p]=20+(x+i)%40;a[p+1]=30;a[p+2]=40;a[p+3]=255;b[p]=a[p];b[p+1]=110;b[p+2]=80;b[p+3]=255;}
  g.context->UpdateSubresource(source.texture.Get(),0,nullptr,a.data(),w*4,0);g.context->UpdateSubresource(enhanced.texture.Get(),0,nullptr,b.data(),w*4,0);
  const auto t=AnalysisRecorder::clock();rec.raw(a.data(),w,h,i,t);
  SubtitleSnapshot caption;caption.translated=i<30?"První český titulek\nDruhý řádek":i>=60?"Další věta se zpožděním.":"";caption.displaySource="English source";caption.expiresAt=t+1;caption.observedAt=t;
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();ImGui::Render();
  output.render(g,w,h,source.view.Get(),enhanced.view.Get(),.5f,nullptr,caption,t);rec.image(g,output.image.texture.Get(),'E',w,h,i,i,i,0,t);
  const bool visible=!caption.translated.empty();if(visible){output.render(g,w,h,source.view.Get(),enhanced.view.Get(),.5f,&settings,caption,t);rec.image(g,output.image.texture.Get(),'F',w,h,i,i,i,0,t);}
  rec.output(t,"\"output_id\":"+std::to_string(i)+",\"before\":[\"R\","+std::to_string(i)+"],\"after\":[\"E\","+std::to_string(i)+"],\"final\":[\""+(visible?"F":"E")+"\","+std::to_string(i)+"],\"subtitle\":"+Diagnostics::quote(caption.translated)+",\"subtitle_source\":\"English source\"");
  g.context->Flush();std::this_thread::sleep_for(std::chrono::milliseconds(i==10?180:17));rec.poll(g);
 }
 rec.finish("Synthetic matched-output test");while(rec.pendingReadbacks){g.context->Flush();std::this_thread::sleep_for(std::chrono::milliseconds(1));rec.poll(g);}rec.worker.join();
 if(rec.dropped||rec.rawCount!=90||rec.outputRows.size()!=90)throw std::runtime_error("Lost output frames");
 std::cout<<rec.folder.string()<<'\n';
 ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
