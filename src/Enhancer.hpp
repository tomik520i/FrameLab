#pragma once
#include "Graphics.hpp"
#include "NrApi.hpp"
#include <d3d11_4.h>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>

// One frame in flight; the UI never waits for inference. The retained source
// and result belong to the same submission, even if newer capture frames drop.
class Enhancer {
public:
 enum class State {Loading,Idle,Working,Ready,Failed,Stopped};
 Enhancer(Graphics& graphics,unsigned width,unsigned height,unsigned maxHeight,
          const std::filesystem::path& app,const std::filesystem::path& runtime);
 ~Enhancer();
 Enhancer(const Enhancer&)=delete;Enhancer& operator=(const Enhancer&)=delete;
 bool submit(ID3D11Texture2D* source,bool reset=false,bool motion=true);
 bool take();
 void requestStop();
 bool stopped()const{return finished.load();}
 State state()const{return phase.load();}
 std::string error()const;
 float processingMs()const{return elapsedMs.load();}
 unsigned width()const{return w;}unsigned height()const{return h;}
 unsigned inferenceWidth()const{return iw;}unsigned inferenceHeight()const{return ih;}
 bool hasResult=false;
 Texture original,result;
private:
 Graphics& g;unsigned w,h,iw,ih;HMODULE library=nullptr;
 NrCreateFn create=nullptr;NrProcessFn process=nullptr;NrDestroyFn destroy=nullptr;
 Texture input,output;ComPtr<ID3D11Fence> inputFence;ComPtr<ID3D11DeviceContext4> context;
 HANDLE inputHandle=nullptr,outputHandle=nullptr,fenceHandle=nullptr;
 uint64_t sequence=0;std::filesystem::path runtimePath,cachePath;
 std::atomic<State> phase{State::Loading};std::atomic<bool> finished{false};std::atomic<float> elapsedMs{0};
 mutable std::mutex mutex;std::condition_variable cv;bool stop=false,queued=false;NrFrame job{};std::string failure;
 std::thread worker;
 void run(NrConfig config);
 void fail(const std::string& text);
};
