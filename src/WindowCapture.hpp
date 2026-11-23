#pragma once
#include "DirectShowCapture.hpp"
#include <Windows.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>
#include <vector>
class WindowCapture {
 std::jthread worker_;mutable std::mutex mutex_;std::string error_;
 std::atomic<bool> active_=false,paused_=false;std::atomic<uint64_t> frames_=0;
 HWND target_=nullptr;DWORD process_=0;int rate_=60;
public:
 struct Window{HWND handle;DWORD process;std::string title;};
 static std::vector<Window> enumerate();static bool supported();
 ~WindowCapture(){stop();}
 void start(HWND target,int adapter,int fps,DirectShowCapture::FrameHandler handler,bool allowOwnProcessForTest=false);
 void stop();bool active()const{return active_;}bool paused()const{return paused_;}
 HWND target()const{return target_;}DWORD process()const{return process_;}uint64_t frames()const{return frames_;}
 std::string error()const{std::lock_guard lock(mutex_);return error_;}
};
