#include "LocalVideoPlayer.hpp"
#include <Windows.h>
#include <atomic>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <wrl/client.h>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cmath>

using Microsoft::WRL::ComPtr;
float ownAudioPeak(const std::wstring& endpoint){
 ComPtr<IMMDeviceEnumerator> e;if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,IID_PPV_ARGS(&e))))return -1;
 ComPtr<IMMDevice> device;if(FAILED(endpoint.empty()?e->GetDefaultAudioEndpoint(eRender,eConsole,&device):e->GetDevice(endpoint.c_str(),&device)))return -1;
 ComPtr<IAudioSessionManager2> manager;if(FAILED(device->Activate(__uuidof(IAudioSessionManager2),CLSCTX_ALL,nullptr,&manager)))return -1;
 ComPtr<IAudioSessionEnumerator> sessions;if(FAILED(manager->GetSessionEnumerator(&sessions)))return -1;int count=0;sessions->GetCount(&count);float peak=-1;
 for(int i=0;i<count;++i){ComPtr<IAudioSessionControl> control;sessions->GetSession(i,&control);ComPtr<IAudioSessionControl2> info;if(!control||FAILED(control.As(&info)))continue;DWORD pid=0;info->GetProcessId(&pid);if(pid!=GetCurrentProcessId())continue;
  ComPtr<IAudioMeterInformation> meter;if(SUCCEEDED(control.As(&meter))){float value=0;if(SUCCEEDED(meter->GetPeakValue(&value)))peak=std::max(peak,value);}
 }return peak;
}
int wmain(int argc,wchar_t** argv){CoInitializeEx(nullptr,COINIT_MULTITHREADED);try{
 if(argc<2)throw std::runtime_error("Usage: local_video_test video.mp4 [silent]");
 bool silent=argc>2&&std::wstring(argv[2])==L"silent";
 std::wstring endpoint;if(argc>2&&std::wstring(argv[2])==L"selected"){auto outputs=LocalVideoPlayer::audioOutputs();if(outputs.size()<2)throw std::runtime_error("No explicit audio endpoint available");endpoint=outputs[1].id;std::cout<<"selected_endpoint="<<outputs[1].name<<std::endl;}
 LocalVideoPlayer player;std::atomic<uint64_t> frames=0;std::atomic<bool> badPixels=false;
 auto callback=[&](const DirectShowCapture::Frame& f,uint64_t){
  if(f.width!=320||f.height!=180||f.stride<1280)badPixels=true;
  // Fixture: red top half, blue bottom half, distinguish vertical orientation.
  auto row=[&](unsigned y){return f.data+(f.bottomUp?f.height-1-y:y)*f.stride;};
  const auto top=row(10)+40,bottom=row(170)+40;
  if(frames==0)std::cout<<"top BGR="<<int(top[0])<<","<<int(top[1])<<","<<int(top[2])<<" bottom="<<int(bottom[0])<<","<<int(bottom[1])<<","<<int(bottom[2])<<std::endl;
  if(top[2]<200||top[0]>30||bottom[0]<200||bottom[2]>30)badPixels=true;
  ++frames;
 };
 player.open(argv[1],endpoint,5,false,callback);
 auto wait=[&](auto predicate,const char* message){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(25);for(;;){auto s=player.snapshot();if(!s.error.empty())throw std::runtime_error(s.error);if(predicate(s))return s;if(std::chrono::steady_clock::now()>end)throw std::runtime_error(message);Sleep(5);}};
 auto state=wait([&](auto s){return s.playing&&frames>24;},"No clocked video frames");
 std::cout<<"fps="<<state.fps<<" audio="<<state.audio<<" duration="<<state.duration<<" position="<<state.position<<" frames="<<frames<<std::endl;
 if(std::abs(state.fps-24)>.01||state.audio!=!silent||std::abs(state.duration-8)>.1)throw std::runtime_error("Wrong native media metadata");
 if(std::abs(double(frames)/24-state.position)>.2)throw std::runtime_error("Video did not follow the presentation clock");
 if(!silent){float peak=0;for(int i=0;i<20;++i){peak=std::max(peak,ownAudioPeak(endpoint));Sleep(10);}std::cout<<"own_audio_session_peak="<<peak<<std::endl;if(peak<=.00001f)throw std::runtime_error("No decoded sound in this process audio session");}
 if(badPixels)throw std::runtime_error("Decoded colors/orientation are wrong");
 player.pause(true);state=wait([](auto s){return s.paused;},"Pause failed");auto count=frames.load();auto position=state.position;Sleep(350);
 if(frames!=count||std::abs(player.snapshot().position-position)>.05)throw std::runtime_error("Pause advanced audio/video");
 player.volume(0,true);player.seek(5);wait([](auto s){return s.generation>1&&s.position>=4.95&&s.paused;},"Paused seek failed");
 player.pause(false);wait([&](auto s){return s.playing&&s.position>5.4;},"Resume failed");
 player.seek(7.7);wait([](auto s){return s.ended;},"EOF did not stop");
 player.loop(true);wait([](auto s){return s.playing&&s.position<2&&s.generation>=4;},"Loop did not restart");
 player.loop(false);player.stop();count=frames.load();Sleep(100);if(frames!=count)throw std::runtime_error("Frames arrived after shutdown");
 player.open(L"Z:/nonexistent-framelab-test.mp4",L"",0,true,callback);
 auto until=std::chrono::steady_clock::now()+std::chrono::seconds(10);while(player.snapshot().error.empty()&&std::chrono::steady_clock::now()<until)Sleep(5);
 if(player.snapshot().error.empty())throw std::runtime_error("Missing file not reported");player.stop();
 std::cout<<"PASS: native cadence, orientation, audio topology, pause, seek, EOF, loop, mute commands, shutdown, missing file\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
