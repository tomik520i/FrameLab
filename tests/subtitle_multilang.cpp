#include "SubtitleRaster.hpp"
#include "SubtitleLanguages.hpp"
#include "SubtitleTranslator.hpp"
#include "SubtitleState.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

int wmain(int argc,wchar_t** argv){try{
 if(argc!=2)throw std::runtime_error("Usage: subtitle_multilang_test OUTPUT_DIR");
 std::filesystem::path out=argv[1];std::filesystem::create_directories(out);
 Graphics graphics;graphics.initDevice();subtitleRaster.device=graphics.device;
 struct Case{const char* code;const char* text;};
 const Case cases[]={
  {"en","Do not open the red door. We have 5 minutes."},
  {"cs","Neotvírej červené dveře. Máme 5 minut."},
  {"sk","Neotváraj červené dvere. Máme 5 minút."},
  {"de","Öffne nicht die rote Tür. Wir haben 5 Minuten."},
  {"fr","N'ouvre pas la porte rouge. Il reste 5 minutes."},
  {"es","No abras la puerta roja. Tenemos 5 minutos."},
  {"pl","Nie otwieraj czerwonych drzwi. Mamy 5 minut."},
  {"uk","Не відчиняй червоні двері. У нас є 5 хвилин."},
  {"ru","Не открывай красную дверь. У нас есть 5 минут."},
  {"ja","赤いドアを開けないでください。残り5分です。"},
  {"ko","빨간 문을 열지 마세요. 5분 남았습니다."},
  {"zh-CN","不要打开红色的门。我们还有5分钟。"},
  {"zh-TW","不要打開紅色的門。我們還有5分鐘。"},
  {"ar","لا تفتح الباب الأحمر. لدينا 5 دقائق."},
  {"el","Μην ανοίξεις την κόκκινη πόρτα. Έχουμε 5 λεπτά."},
  {"hi","लाल दरवाज़ा मत खोलो। हमारे पास 5 मिनट हैं।"},
  {"tr","Kırmızı kapıyı açma. 5 dakikamız var."},
  {"vi","Đừng mở cửa màu đỏ. Chúng ta có 5 phút."},
  {"th","อย่าเปิดประตูสีแดง เรามีเวลา 5 นาที"},
  {"fa","در قرمز را باز نکن. ما 5 دقیقه وقت داریم."},
  {"ta","சிவப்பு கதவைத் திறக்காதே. நமக்கு 5 நிமிடங்கள் உள்ளன."},
  {"te","ఎర్ర తలుపు తెరవవద్దు. మనకు 5 నిమిషాలు ఉన్నాయి."}
 };
 std::ofstream manifest(out/L"fixtures.tsv");
 int frame=0;unsigned missing=0;
 for(auto c:cases){
  if(SubtitleState::normalize(c.text)!=c.text)throw std::runtime_error("Unicode OCR filter dropped "+std::string(c.code));
  const auto language=subtitleLanguage(c.code);
  for(int size:{36,60}){
   auto image=subtitleRaster.get(c.text,float(size),1150,c.code,language->rtl,++frame);
   if(!image)throw std::runtime_error(subtitleRaster.error);
   missing+=image->missingGlyphs;
   D3D11_TEXTURE2D_DESC desc{};image->texture.texture->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
   ComPtr<ID3D11Texture2D> staging;check(graphics.device->CreateTexture2D(&desc,nullptr,&staging),"Readback");graphics.context->CopyResource(staging.Get(),image->texture.texture.Get());
   D3D11_MAPPED_SUBRESOURCE mapped{};check(graphics.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped),"Map");
   std::vector<unsigned char> pixels(1280*720*4,24);for(size_t i=3;i<pixels.size();i+=4)pixels[i]=255;
   unsigned top=540,left=(1280-image->width)/2;
   if(top+image->height>=720)top=720-image->height-20;
   for(unsigned y=0;y<image->height;++y)for(unsigned x=0;x<image->width;++x){auto p=(unsigned char*)mapped.pData+y*mapped.RowPitch+x*4;auto d=pixels.data()+((y+top)*1280+x+left)*4;for(int k=0;k<3;++k)d[k]=(p[2-k]*p[3]+24*(255-p[3]))/255;}
   graphics.context->Unmap(staging.Get(),0);
   const auto name=std::string(c.code)+"-"+std::to_string(size)+".bgra";
   std::ofstream file(out/name,std::ios::binary);file.write((char*)pixels.data(),pixels.size());
   manifest<<c.code<<'\t'<<size<<'\t'<<name<<'\t'<<image->missingGlyphs<<'\t'<<c.text<<'\n';
  }
 }
 if(!SubtitleState::normalize("5 / 100").empty())throw std::runtime_error("Numeric HUD accepted");
 if(SubtitleState::normalize("止").empty()||SubtitleState::normalize("لا").empty())throw std::runtime_error("Short commands dropped");
 SubtitleSettings s;s.model=4;s.sourceLanguage="ja";s.targetLanguage="ar";s.save(out/L"settings.txt");SubtitleSettings loaded;loaded.load(out/L"settings.txt");if(s!=loaded)throw std::runtime_error("Language persistence failed");
 if(subtitlePairSupported(1,"ja","cs")||subtitlePairSupported(4,"ja","ja")||!subtitlePairSupported(4,"ja","cs"))throw std::runtime_error("Wrong model pair acceptance");
 std::cout<<"RENDERED 44 fixtures, missing glyphs="<<missing<<"; Unicode filtering, pair validation and persistence PASS\n";
 if(missing)throw std::runtime_error("Windows missing script fonts; see fixtures.tsv");
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
