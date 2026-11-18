#pragma once
#include "DirectShowCapture.hpp"
#include "Graphics.hpp"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <chrono>
#include <thread>
#include <stop_token>
#include <filesystem>
#include <vector>
#include <algorithm>

// Local video fixture playback using Windows' decoder. No Python, FFmpeg,
// downloads, or multi-gigabyte uncompressed file is needed at application runtime.
inline void replayVideoFile(const std::filesystem::path& path,double fpsLimit,
                           const DirectShowCapture::FrameHandler& handler,std::stop_token stop){
 struct MediaSession {MediaSession(){check(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"Replay COM");check(MFStartup(MF_VERSION,MFSTARTUP_LITE),"Replay Media Foundation");}~MediaSession(){MFShutdown();CoUninitialize();}} session;
 ComPtr<IMFAttributes> attributes;check(MFCreateAttributes(&attributes,2),"Replay attributes");check(attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING,TRUE),"Replay RGB conversion");
 ComPtr<IMFSourceReader> reader;check(MFCreateSourceReaderFromURL(path.c_str(),attributes.Get(),&reader),"Open sample video");
 check(reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,FALSE),"Replay stream selection");check(reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM,TRUE),"Replay video selection");
 ComPtr<IMFMediaType> requested;check(MFCreateMediaType(&requested),"Replay output type");requested->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);requested->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_RGB32);check(reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,nullptr,requested.Get()),"Replay BGRA output");
 ComPtr<IMFMediaType> format;check(reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,&format),"Replay actual format");UINT32 w=0,h=0;check(MFGetAttributeSize(format.Get(),MF_MT_FRAME_SIZE,&w,&h),"Replay dimensions");
 if(!w||!h||w>7680||h>4320)throw std::runtime_error("Unsupported sample video dimensions");
 UINT32 strideBits=0;LONG stride=LONG(w*4);if(SUCCEEDED(format->GetUINT32(MF_MT_DEFAULT_STRIDE,&strideBits)))stride=LONG(strideBits);
 std::vector<unsigned char> pixels(size_t(w)*h*4);using Clock=std::chrono::steady_clock;auto start=Clock::now();LONGLONG first=-1,last=-1;unsigned emptyLoops=0;bool hadFrame=false;
 while(!stop.stop_requested()){
  DWORD flags=0;LONGLONG pts=0;ComPtr<IMFSample> sample;check(reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,0,nullptr,&flags,&pts,&sample),"Decode sample video");
  if(flags&MF_SOURCE_READERF_ERROR)throw std::runtime_error("Video decoder reported an error");
  if(flags&MF_SOURCE_READERF_ENDOFSTREAM){if(!hadFrame&&++emptyLoops>1)throw std::runtime_error("Sample video contains no decodable frames");PROPVARIANT position{};position.vt=VT_I8;position.hVal.QuadPart=0;check(reader->SetCurrentPosition(GUID_NULL,position),"Loop sample video");first=last=-1;hadFrame=false;continue;}
  if(!sample)continue;hadFrame=true;if(first<0){first=pts;start=Clock::now();}
  if(last>=0&&pts-last<LONGLONG(10000000.0/fpsLimit)-1000)continue;
  const auto deadline=start+std::chrono::nanoseconds((pts-first)*100);
  while(!stop.stop_requested()&&Clock::now()<deadline)std::this_thread::sleep_for(std::min(std::chrono::milliseconds(5),std::chrono::duration_cast<std::chrono::milliseconds>(deadline-Clock::now())));
  if(stop.stop_requested())break;
  ComPtr<IMFMediaBuffer> buffer;check(sample->ConvertToContiguousBuffer(&buffer),"Replay buffer");ComPtr<IMF2DBuffer> twoD;BYTE* scan=nullptr;LONG pitch=stride;
  if(SUCCEEDED(buffer.As(&twoD))&&SUCCEEDED(twoD->Lock2D(&scan,&pitch))){
   if(std::abs(pitch)<LONG(w*4)){twoD->Unlock2D();throw std::runtime_error("Invalid decoded row stride");}
   for(UINT32 y=0;y<h;++y)memcpy(pixels.data()+size_t(y)*w*4,scan+ptrdiff_t(y)*pitch,w*4);twoD->Unlock2D();
  }else{DWORD bytes=0;check(buffer->Lock(&scan,nullptr,&bytes),"Replay buffer lock");if(bytes<size_t(std::abs(stride))*h){buffer->Unlock();throw std::runtime_error("Truncated decoded frame");}if(stride<0)scan+=size_t(h-1)*(-stride);for(UINT32 y=0;y<h;++y)memcpy(pixels.data()+size_t(y)*w*4,scan+ptrdiff_t(y)*stride,w*4);buffer->Unlock();}
  DirectShowCapture::Frame frame{};frame.width=frame.sampleWidth=w;frame.height=frame.sampleHeight=h;frame.stride=w*4;frame.data=pixels.data();frame.dataSize=pixels.size();frame.timestamp100ns=pts;handler(frame);last=pts;
 }
}
