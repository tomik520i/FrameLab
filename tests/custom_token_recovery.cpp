#include "SubtitleTranslator.hpp"
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <stdexcept>

void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
double seconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
int wmain(int argc,wchar_t** argv){try{
 require(argc==3,"Usage: custom_token_recovery APP PORT");
 const std::filesystem::path app=argv[1];
 SubtitleSettings settings;settings.provider=2;settings.customUrl="http://127.0.0.1:"+std::to_string(std::stoi(argv[2]))+"/v1";settings.customModel="test-model";
 settings.customMaxTokens=1000;settings.enabled=true;settings.conversationMemory=true;settings.selectedMemory=false;settings.intervalMs=200;
 {std::ofstream probe(app/L"write-probe.txt");probe<<"probe";probe.flush();require(probe.good(),"Native test process cannot write its fixture directory");}
 settings.save(app/L"settings-test-1000.txt");SubtitleSettings loaded;loaded.load(app/L"settings-test-1000.txt");
 require(std::filesystem::exists(app/L"settings-test-1000.txt"),"Settings file not created");
 require(loaded.customMaxTokens==1000,"1000-token setting did not persist");
 settings.customMaxTokens=200000;settings.sanitize();require(settings.customMaxTokens==200000,"Hidden token cap remains");
 settings.save(app/L"settings-test-200000.txt");loaded.load(app/L"settings-test-200000.txt");require(loaded.customMaxTokens==200000,"Large token count did not persist");
 settings.customMaxTokens=1000;
 SubtitleTranslator worker(app);worker.configure(settings);std::vector<unsigned char> pixels(640*360*4,0);
 auto phase=[&](const char* source,bool bad,const char* expected){
  {std::ofstream f(app/L"test-source.txt");f<<source;}
  auto started=seconds();bool passed=false;
  while(seconds()-started<15){worker.submit(pixels.data(),640,360,seconds());std::this_thread::sleep_for(std::chrono::milliseconds(80));auto state=worker.snapshot();
   if(bad?state.performance.translationErrors>0:state.translated==expected){passed=true;break;}
  }
  require(passed,"Translation pipeline did not progress");
  std::cout<<source<<" passed\n";
 };
 phase("Clerk: Don't hurt me!",true,"");
 auto failed=worker.snapshot();require(failed.translated.empty(),"Incomplete output was displayed");require(failed.activeModel>=0,"Healthy worker was unloaded");
 phase("Jason: Keep moving.",false,"Jason: Pokračuj.");
 settings.customMaxTokens=4096;worker.configure(settings);
 phase("Lucia: Stay here.",false,"Lucia: Zůstaň tady.");
 const auto done=worker.snapshot();require(done.performance.translationErrors==1,"Unexpected extra errors");require(done.performance.translations>=2,"Next subtitles not translated");
 settings.enabled=false;worker.configure(settings);
 std::cout<<"PASS: real C++ pipeline + Python translator + HTTP mock; truncated response skipped, next subtitle displayed without toggling, token reconfiguration and save/load\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
