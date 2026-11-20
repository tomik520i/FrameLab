#include "SubtitleStylePicker.hpp"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

// Actual ImGui mouse events + pixels rendered into our own offscreen D3D target.
// Does not create/capture a desktop window or inject Windows mouse/keyboard input.
int wmain(int argc,wchar_t** argv){try{
 if(argc!=4)throw std::runtime_error("Usage: subtitle_style_ui_test APP FIXTURES OUTPUT_BMP");
 constexpr unsigned w=1280,h=900,sw=1280,sh=720;
 std::vector<unsigned char> source(size_t(sw)*sh*4);std::ifstream input(std::filesystem::path(argv[2])/L"mixed-styles.bgra",std::ios::binary);if(!input.read(reinterpret_cast<char*>(source.data()),source.size()))throw std::runtime_error("Fixture missing");
 std::array<float,4> word;std::ifstream selection(std::filesystem::path(argv[2])/L"selection.txt");for(auto& v:word)selection>>v;
 Graphics g;g.initDevice();Texture target;target.create(g.device.Get(),w,h);ComPtr<ID3D11RenderTargetView> rtv;check(g.device->CreateRenderTargetView(target.texture.Get(),nullptr,&rtv),"RTV");
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={float(w),float(h)};io.DeltaTime=1.f/60;ui::initialize();ImGui_ImplDX11_Init(g.device.Get(),g.context.Get());
 SubtitleTranslator worker(argv[1]);SubtitleSettings settings;settings.left=0;settings.top=.35f;settings.right=1;settings.bottom=.98f;
 SubtitleStylePicker picker;picker.requested=true;ImVec4 canvas{};bool applied=false;ImVec2 useButton{};
 auto frame=[&]{
  ImGui_ImplDX11_NewFrame();ImGui::NewFrame();applied=picker.draw(g,worker,settings,source,sw,sh,false,argv[1]);
  if(auto window=ImGui::FindWindowByName("Select subtitle appearance"))useButton={window->DC.CursorStartPos.x+70,window->DC.CursorPosPrevLine.y+ImGui::GetFrameHeight()*.5f};
  ImGui::Render();const float clear[]={.05f,.07f,.05f,1};g.context->ClearRenderTargetView(rtv.Get(),clear);g.context->OMSetRenderTargets(1,rtv.GetAddressOf(),nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  auto data=ImGui::GetDrawData();for(int l=0;l<data->CmdListsCount;++l){auto list=data->CmdLists[l];for(const auto& cmd:list->CmdBuffer){if(cmd.ElemCount!=6||cmd.GetTexID()==io.Fonts->TexID)continue;float x0=1e9f,y0=1e9f,x1=0,y1=0;for(unsigned i=0;i<6;++i){auto p=list->VtxBuffer[list->IdxBuffer[cmd.IdxOffset+i]+cmd.VtxOffset].pos;x0=std::min(x0,p.x);y0=std::min(y0,p.y);x1=std::max(x1,p.x);y1=std::max(y1,p.y);}if(x1-x0>200&&y1-y0>100)canvas={x0,y0,x1,y1};}}
 };
 frame();frame();if(canvas.z<=canvas.x)throw std::runtime_error("Sample image missing from modal");
 auto point=[&](float x,float y){return ImVec2{canvas.x+x*(canvas.z-canvas.x),canvas.y+y*(canvas.w-canvas.y)};};
 auto a=point(word[0],word[1]),b=point(word[2],word[3]);io.AddMousePosEvent(a.x,a.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMousePosEvent(b.x,b.y);frame();io.AddMouseButtonEvent(0,false);frame();
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(40);
 while(std::chrono::steady_clock::now()<deadline){frame();auto s=worker.styleSample();if(s.id&&!s.busy){if(!s.error.empty())throw std::runtime_error(s.error);break;}std::this_thread::sleep_for(std::chrono::milliseconds(20));}
 auto sample=worker.styleSample();if(!sample.profile.enabled||sample.text.find("Follow the road")==std::string::npos)throw std::runtime_error("Actual ImGui drag failed to learn the word");
 frame();
 D3D11_TEXTURE2D_DESC desc{};target.texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;check(g.device->CreateTexture2D(&desc,nullptr,&staging),"Staging");g.context->CopyResource(staging.Get(),target.texture.Get());D3D11_MAPPED_SUBRESOURCE map{};check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"Map");
 BITMAPFILEHEADER fh{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(BITMAPINFOHEADER);fh.bfSize=fh.bfOffBits+w*h*4;BITMAPINFOHEADER ih{};ih.biSize=sizeof(ih);ih.biWidth=w;ih.biHeight=-int(h);ih.biPlanes=1;ih.biBitCount=32;ih.biCompression=BI_RGB;
 std::ofstream output(argv[3],std::ios::binary);output.write(reinterpret_cast<char*>(&fh),sizeof(fh));output.write(reinterpret_cast<char*>(&ih),sizeof(ih));for(unsigned y=0;y<h;++y)output.write(static_cast<const char*>(map.pData)+size_t(y)*map.RowPitch,w*4);g.context->Unmap(staging.Get(),0);output.close();
 io.AddMousePosEvent(useButton.x,useButton.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();
 if(!applied||!settings.style.enabled||settings.left!=0||settings.top!=.35f||settings.right!=1||settings.bottom!=.98f)throw std::runtime_error("Use style button failed or changed reading area to word size");
 if(settings.style.sampleText!="Follow"||!settings.style.sampleId)throw std::runtime_error("Selected word evidence not saved");
 SubtitleStyleEvidence savedEvidence;if(!savedEvidence.load(g,argv[1],settings.style.sampleId))throw std::runtime_error("Saved crop failed to reload");
 settings.enabled=true;settings.inspectOcr=true;worker.configure(settings);auto until=std::chrono::steady_clock::now()+std::chrono::seconds(40);
 while(worker.snapshot().ocrLines.empty()&&std::chrono::steady_clock::now()<until){const auto t=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();worker.submit(source.data(),sw,sh,t);std::this_thread::sleep_for(std::chrono::milliseconds(50));}
 if(worker.snapshot().ocrLines.size()<6)throw std::runtime_error("Inspector live rows missing");settings.inspectOcr=false;
 SubtitleInspector inspector;inspector.open=true;auto inspectorFrame=[&]{ImGui_ImplDX11_NewFrame();ImGui::NewFrame();inspector.draw(g,argv[1],worker,settings,sh,false);ImGui::Render();const float clear[]={.05f,.07f,.05f,1};g.context->ClearRenderTargetView(rtv.Get(),clear);g.context->OMSetRenderTargets(1,rtv.GetAddressOf(),nullptr);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());};inspectorFrame();inspectorFrame();if(!settings.inspectOcr)throw std::runtime_error("Inspector did not enable measurements");
 g.context->CopyResource(staging.Get(),target.texture.Get());check(g.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"Inspector map");auto inspectorPath=std::filesystem::path(argv[3]);inspectorPath.replace_filename(L"inspector.bmp");std::ofstream inspectorImage(inspectorPath,std::ios::binary);inspectorImage.write(reinterpret_cast<char*>(&fh),sizeof(fh));inspectorImage.write(reinterpret_cast<char*>(&ih),sizeof(ih));for(unsigned y=0;y<h;++y)inspectorImage.write(static_cast<const char*>(map.pData)+size_t(y)*map.RowPitch,w*4);g.context->Unmap(staging.Get(),0);inspectorImage.close();
 inspector.open=false;ImGui_ImplDX11_NewFrame();ImGui::NewFrame();inspector.draw(g,argv[1],worker,settings,sh,false);if(settings.inspectOcr)throw std::runtime_error("Inspector left extra work enabled");ImGui::Render();
 ImGui_ImplDX11_Shutdown();ImGui::DestroyContext();std::cout<<"PASS: real ImGui word drag, worker preview, rendered decisions, confirmation, unchanged reading area\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
