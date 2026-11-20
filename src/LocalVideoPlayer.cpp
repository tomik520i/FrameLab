#include "LocalVideoPlayer.hpp"
#include "VlcRuntime.hpp"
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <wrl/client.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <cmath>
#include <malloc.h>
using Microsoft::WRL::ComPtr;
namespace {
void require(HRESULT r,const char* context){if(FAILED(r)){char text[256];snprintf(text,sizeof(text),"%s (0x%08X). Try an unprotected MP4 with H.264/AAC, or install the required Windows codec.",context,unsigned(r));throw std::runtime_error(text);}}
std::string utf8(const std::wstring& value){auto n=WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),nullptr,0,nullptr,nullptr);std::string out(n,0);WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),out.data(),n,nullptr,nullptr);return out;}
struct Platform {
 Platform(){require(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"Video COM");auto r=MFStartup(MF_VERSION);if(FAILED(r)){CoUninitialize();require(r,"Video Media Foundation");}}
 ~Platform(){MFShutdown();CoUninitialize();}
};
struct Shared {
 mutable std::mutex mutex,delivery;
 LocalVideoPlayer::Snapshot state;
 LocalVideoPlayer::Handler handler;
 std::atomic<bool> stopping{false};
 bool wantPause=false,wantLoop=false;double wantSeek=-1;float wantVolume=.75f;
 int wantAudioTrack=-2,wantSubtitleTrack=-2;
 std::atomic<uint64_t> generation{1};unsigned width=0,height=0;bool accept=false;
};
class VideoSink final:public IMFSampleGrabberSinkCallback {
 std::atomic<ULONG> refs{1};std::shared_ptr<Shared> shared;
public:
 explicit VideoSink(std::shared_ptr<Shared> s):shared(std::move(s)){}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(id==__uuidof(IUnknown)||id==__uuidof(IMFClockStateSink)||id==__uuidof(IMFSampleGrabberSinkCallback)){*out=static_cast<IMFSampleGrabberSinkCallback*>(this);AddRef();return S_OK;}return E_NOINTERFACE;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}
 ULONG STDMETHODCALLTYPE Release()override{auto r=--refs;if(!r)delete this;return r;}
 HRESULT STDMETHODCALLTYPE OnSetPresentationClock(IMFPresentationClock*)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE OnClockStart(MFTIME,LONGLONG)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE OnClockStop(MFTIME)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE OnClockPause(MFTIME)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE OnClockRestart(MFTIME)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE OnClockSetRate(MFTIME,float)override{return S_OK;}
 HRESULT STDMETHODCALLTYPE OnShutdown()override{return S_OK;}
 HRESULT STDMETHODCALLTYPE OnProcessSample(REFGUID,DWORD,LONGLONG pts,LONGLONG,const BYTE* data,DWORD bytes)override{
  try{
   std::lock_guard delivery(shared->delivery);
   if(!shared->accept||shared->stopping)return S_OK;
   DirectShowCapture::Frame frame{};frame.width=frame.sampleWidth=shared->width;frame.height=frame.sampleHeight=shared->height;
   frame.stride=frame.width*4;frame.data=data;frame.dataSize=bytes;frame.timestamp100ns=std::max<LONGLONG>(0,pts);
   // The explicit Video Processor MFT produces RGB32 in top-down order.
   frame.bottomUp=false;
   if(bytes<size_t(frame.stride)*frame.height)return MF_E_BUFFERTOOSMALL;
   shared->handler(frame,shared->generation.load());
   {std::lock_guard lock(shared->mutex);++shared->state.frames;shared->state.generation=shared->generation;}
   return S_OK;
  }catch(...){return E_FAIL;}
 }
};
struct Session {
 ComPtr<IMFMediaSession> session;ComPtr<IMFMediaSource> source;
 ~Session(){if(session)session->Shutdown();if(source)source->Shutdown();}
 void wait(MediaEventType expected,const std::shared_ptr<Shared>& shared){
  auto until=std::chrono::steady_clock::now()+std::chrono::seconds(20);
  while(!shared->stopping){
   ComPtr<IMFMediaEvent> e;auto r=session->GetEvent(MF_EVENT_FLAG_NO_WAIT,&e);
   if(r==MF_E_NO_EVENTS_AVAILABLE){if(std::chrono::steady_clock::now()>until)throw std::runtime_error("Video session timed out");Sleep(2);continue;}
   require(r,"Video session event");HRESULT status=S_OK;require(e->GetStatus(&status),"Video event status");require(status,"Video decoder / audio renderer");
   MediaEventType type;require(e->GetType(&type),"Video event type");
   if(type==expected){if(type==MESessionTopologyStatus){UINT32 topologyStatus=0;if(FAILED(e->GetUINT32(MF_EVENT_TOPOLOGY_STATUS,&topologyStatus))||topologyStatus!=MF_TOPOSTATUS_READY)continue;}return;}
  }
 }
 void start(double at,const std::shared_ptr<Shared>& shared){PROPVARIANT pos{};if(at>=0){pos.vt=VT_I8;pos.hVal.QuadPart=LONGLONG(at*10000000);}require(session->Start(&GUID_NULL,&pos),"Start video");wait(MESessionStarted,shared);}
};
void run(const std::shared_ptr<Shared>& shared,const std::filesystem::path& path,const std::wstring& endpoint){
 try{
  Platform platform;Session media;
  ComPtr<IMFSourceResolver> resolver;require(MFCreateSourceResolver(&resolver),"Video resolver");MF_OBJECT_TYPE type;ComPtr<IUnknown> object;
  require(resolver->CreateObjectFromURL(path.c_str(),MF_RESOLUTION_MEDIASOURCE,nullptr,&type,&object),"Open local video");require(object.As(&media.source),"Video source");
  ComPtr<IMFPresentationDescriptor> presentation;require(media.source->CreatePresentationDescriptor(&presentation),"Video streams");
  PROPVARIANT duration{};double seconds=0;if(SUCCEEDED(presentation->GetItem(MF_PD_DURATION,&duration))&&duration.vt==VT_UI8)seconds=duration.uhVal.QuadPart/10000000.;PropVariantClear(&duration);
  ComPtr<IMFTopology> topology;require(MFCreateTopology(&topology),"Video topology");
  DWORD count=0;require(presentation->GetStreamDescriptorCount(&count),"Video stream count");bool video=false,audio=false;double fps=0;
  // First deselect all streams; only the first video/audio track is supported.
  for(DWORD i=0;i<count;++i)require(presentation->DeselectStream(i),"Video stream deselection");
  for(DWORD i=0;i<count;++i){
   BOOL selected=FALSE;ComPtr<IMFStreamDescriptor> stream;require(presentation->GetStreamDescriptorByIndex(i,&selected,&stream),"Video stream");
   ComPtr<IMFMediaTypeHandler> handler;require(stream->GetMediaTypeHandler(&handler),"Video stream type");GUID major;require(handler->GetMajorType(&major),"Video major type");
   if((major!=MFMediaType_Video||video)&&(major!=MFMediaType_Audio||audio))continue;
   ComPtr<IMFActivate> activation;ComPtr<IMFTopologyNode> conversion;
   if(major==MFMediaType_Video){
    ComPtr<IMFMediaType> native;require(handler->GetCurrentMediaType(&native),"Video native type");UINT32 w=0,h=0;require(MFGetAttributeSize(native.Get(),MF_MT_FRAME_SIZE,&w,&h),"Video dimensions");
    if(!w||!h||w>7680||h>4320)throw std::runtime_error("Unsupported video size (maximum 7680 x 4320)");
    UINT32 n=0,d=0;if(SUCCEEDED(MFGetAttributeRatio(native.Get(),MF_MT_FRAME_RATE,&n,&d))&&d)fps=double(n)/d;
    shared->width=w;shared->height=h;
    ComPtr<IMFMediaType> output;require(MFCreateMediaType(&output),"Video RGB type");output->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);output->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_RGB32);
    MFSetAttributeSize(output.Get(),MF_MT_FRAME_SIZE,w,h);output->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive);
    MFSetAttributeRatio(output.Get(),MF_MT_FRAME_RATE,n?n:30,d?d:1);MFSetAttributeRatio(output.Get(),MF_MT_PIXEL_ASPECT_RATIO,1,1);
    output->SetUINT32(MF_MT_DEFAULT_STRIDE,w*4);output->SetUINT32(MF_MT_SAMPLE_SIZE,w*h*4);output->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES,TRUE);output->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
    ComPtr<IMFTransform> converter;require(CoCreateInstance(CLSID_VideoProcessorMFT,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&converter)),"Video color converter");
    ComPtr<IMFMediaType> decoded;require(MFCreateMediaType(&decoded),"Decoded type");
    decoded->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);decoded->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_NV12);
    MFSetAttributeSize(decoded.Get(),MF_MT_FRAME_SIZE,w,h);decoded->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive);
    MFSetAttributeRatio(decoded.Get(),MF_MT_FRAME_RATE,n?n:30,d?d:1);MFSetAttributeRatio(decoded.Get(),MF_MT_PIXEL_ASPECT_RATIO,1,1);
    decoded->SetUINT32(MF_MT_DEFAULT_STRIDE,w);decoded->SetUINT32(MF_MT_SAMPLE_SIZE,w*h*3/2);decoded->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES,TRUE);decoded->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
    require(converter->SetInputType(0,decoded.Get(),0),"Video converter input");require(converter->SetOutputType(0,output.Get(),0),"Video converter output");
    require(MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE,&conversion),"Video conversion node");conversion->SetObject(converter.Get());

    ComPtr<IMFSampleGrabberSinkCallback> sink;sink.Attach(new VideoSink(shared));
    require(MFCreateSampleGrabberSinkActivate(output.Get(),sink.Get(),&activation),"Clocked video sink");
    require(activation->SetUINT32(MF_SAMPLEGRABBERSINK_IGNORE_CLOCK,FALSE),"Video clock synchronization");video=true;
   }else{
    require(MFCreateAudioRendererActivate(&activation),"Audio output");
    if(!endpoint.empty())require(activation->SetString(MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID,endpoint.c_str()),"Selected audio device");audio=true;
   }
   require(presentation->SelectStream(i),"Select local media stream");
   ComPtr<IMFTopologyNode> source,sink;require(MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE,&source),"Video source node");
   source->SetUnknown(MF_TOPONODE_SOURCE,media.source.Get());source->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR,presentation.Get());source->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR,stream.Get());
   require(MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE,&sink),"Video sink node");sink->SetObject(activation.Get());sink->SetUINT32(MF_TOPONODE_STREAMID,0);
   require(topology->AddNode(source.Get()),"Add video source");require(topology->AddNode(sink.Get()),"Add video sink");if(conversion){require(topology->AddNode(conversion.Get()),"Add color converter");require(source->ConnectOutput(0,conversion.Get(),0),"Connect decoder");require(conversion->ConnectOutput(0,sink.Get(),0),"Connect video RGB");}
   else require(source->ConnectOutput(0,sink.Get(),0),"Connect audio stream");
  }
  if(!video)throw std::runtime_error("This file has no supported video track");
  require(MFCreateMediaSession(nullptr,&media.session),"Local media session");require(media.session->SetTopology(0,topology.Get()),"Resolve video codecs");media.wait(MESessionTopologyStatus,shared);
  ComPtr<IMFClock> clock;require(media.session->GetClock(&clock),"Video clock");ComPtr<IMFPresentationClock> presentationClock;require(clock.As(&presentationClock),"Presentation clock");
  ComPtr<IMFSimpleAudioVolume> audioVolume;
  if(audio)require(MFGetService(media.session.Get(),MR_POLICY_VOLUME_SERVICE,IID_PPV_ARGS(&audioVolume)),"Audio volume control");
  float appliedVolume=-1;{std::lock_guard lock(shared->mutex);if(audioVolume){require(audioVolume->SetMasterVolume(shared->wantVolume),"Audio volume");appliedVolume=shared->wantVolume;}}
  {std::lock_guard delivery(shared->delivery);shared->accept=true;}
  media.start(0,shared);bool paused=false,ended=false;
  {std::lock_guard lock(shared->mutex);auto& s=shared->state;s.opening=false;s.playing=true;s.audio=audio;s.width=shared->width;s.height=shared->height;s.duration=seconds;s.fps=fps;}
  while(!shared->stopping){
   bool wantPause,wantLoop;double seek;float volume;
   {std::lock_guard lock(shared->mutex);wantPause=shared->wantPause;wantLoop=shared->wantLoop;seek=shared->wantSeek;shared->wantSeek=-1;volume=shared->wantVolume;}
   if(audioVolume&&volume!=appliedVolume){require(audioVolume->SetMasterVolume(volume),"Audio volume");appliedVolume=volume;}
   for(;;){ComPtr<IMFMediaEvent> e;auto r=media.session->GetEvent(MF_EVENT_FLAG_NO_WAIT,&e);if(r==MF_E_NO_EVENTS_AVAILABLE)break;require(r,"Playback event");HRESULT status=S_OK;e->GetStatus(&status);require(status,"Playback");MediaEventType event;e->GetType(&event);if(event==MESessionEnded)ended=true;}
   if(ended&&wantLoop){seek=0;wantPause=false;}
   if(seek>=0){
    // Suppress old callbacks while the old clock/decoder are being flushed.
    {std::lock_guard delivery(shared->delivery);shared->accept=false;}
    if(!paused&&!ended){require(media.session->Pause(),"Pause before seeking");media.wait(MESessionPaused,shared);}
    ++shared->generation;
    {std::lock_guard delivery(shared->delivery);shared->accept=true;}
    media.start(std::clamp(seek,0.,seconds>0?std::max(0.,seconds-1./(fps>0?fps:30)):seek),shared);paused=false;ended=false;
    if(wantPause){
     // Let the clock deliver one sought frame before pausing. Otherwise a
     // paused seek can retain the old image indefinitely in the video sink.
     auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
     while(!shared->stopping){
      {std::lock_guard lock(shared->mutex);if(shared->state.generation==shared->generation.load())break;}
      if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("No video frame after seeking");Sleep(2);
     }
    }
   }
   if(!ended&&wantPause!=paused){if(wantPause){require(media.session->Pause(),"Pause video");media.wait(MESessionPaused,shared);}else media.start(-1,shared);paused=wantPause;}
   MFTIME current=0;presentationClock->GetTime(&current);
   {std::lock_guard lock(shared->mutex);auto& s=shared->state;s.position=ended&&seconds>0?seconds:std::max(0.,current/10000000.);s.playing=!paused&&!ended;s.paused=paused;s.ended=ended;s.loop=wantLoop;}
   Sleep(5);
  }
  {std::lock_guard delivery(shared->delivery);shared->accept=false;}
 }catch(const std::exception& e){std::lock_guard lock(shared->mutex);shared->state.error=e.what();shared->state.opening=shared->state.playing=false;}
 // Shutdown completed before the frame handler is released.
 {std::lock_guard delivery(shared->delivery);shared->accept=false;shared->handler={};}
}
}
namespace {
struct VlcPixels {
 std::shared_ptr<Shared> shared;std::mutex mutex;unsigned char* data=nullptr;unsigned w=0,h=0,pitch=0,nativeWidth=0,nativeHeight=0;std::atomic<int64_t> positionMs=0;
 ~VlcPixels(){_aligned_free(data);}
 static unsigned format(void** opaque,char* chroma,unsigned* width,unsigned* height,unsigned* pitches,unsigned* lines){
  auto& self=*static_cast<VlcPixels*>(*opaque);try{
   // Decoder buffers can include padding (e.g. 320x192 for a 320x180 movie).
   // Request the parsed visible dimensions, not the padded coded rectangle.
   if(self.nativeWidth&&self.nativeHeight){*width=self.nativeWidth;*height=self.nativeHeight;}
   if(!*width||!*height||*width>7680||*height>4320)return 0;
   std::lock_guard lock(self.mutex);self.w=*width;self.h=*height;self.pitch=(self.w*4+31)&~31u;
   _aligned_free(self.data);self.data=static_cast<unsigned char*>(_aligned_malloc(size_t(self.pitch)*self.h,32));if(!self.data)return 0;
   memcpy(chroma,"RV32",4);pitches[0]=self.pitch;lines[0]=self.h;return 1;
  }catch(...){return 0;}
 }
 static void* lock(void* opaque,void** planes){auto& self=*static_cast<VlcPixels*>(opaque);self.mutex.lock();planes[0]=self.data;return opaque;}
 static void unlock(void* opaque,void*,void*const*){static_cast<VlcPixels*>(opaque)->mutex.unlock();}
 static void display(void* opaque,void*){
  auto& self=*static_cast<VlcPixels*>(opaque);try{
   // VLC 3 vmem calls Prepare (lock/copy/unlock) then Display on its vout thread.
   // Protect against format changes and serialize delivery with seek/shutdown.
   std::lock_guard pixels(self.mutex);std::lock_guard delivery(self.shared->delivery);
   if(self.shared->stopping||!self.shared->accept||!self.data)return;
   DirectShowCapture::Frame frame{};frame.width=frame.sampleWidth=self.w;frame.height=frame.sampleHeight=self.h;frame.stride=self.pitch;frame.data=self.data;frame.dataSize=size_t(self.pitch)*self.h;frame.bottomUp=false;
   // LibVLC's display callback exposes no per-picture PTS. This is a playback
   // clock estimate for source discontinuity handling, not an exact frame PTS.
   frame.timestamp100ns=uint64_t(std::max<int64_t>(0,self.positionMs))*10000;
   self.shared->handler(frame,self.shared->generation);
   std::lock_guard state(self.shared->mutex);++self.shared->state.frames;self.shared->state.width=self.w;self.shared->state.height=self.h;self.shared->state.generation=self.shared->generation;
  }catch(...){std::lock_guard state(self.shared->mutex);self.shared->state.error="Video frame delivery failed";}
 }
};
struct VlcSession {
 vlc::Runtime& api;vlc::Instance* instance=nullptr;vlc::Media* media=nullptr;vlc::Player* player=nullptr;
 ~VlcSession(){if(player){api.stop(player);api.playerRelease(player);}if(media)api.mediaRelease(media);if(instance)api.release(instance);}
};
void runVlc(const std::shared_ptr<Shared>& shared,const std::filesystem::path& path,const std::wstring& endpoint,const std::filesystem::path& runtime){
 try{
  if(!std::filesystem::is_regular_file(path))throw std::runtime_error("Video file does not exist");
  vlc::Runtime api(runtime);VlcPixels pixels;pixels.shared=shared;VlcSession media{api};
  const char* options[]={"--quiet","--ignore-config","--no-video-title-show","--no-osd","--no-media-library","--no-metadata-network-access","--no-sub-autodetect-file"};
  media.instance=api.create(int(std::size(options)),options);if(!media.instance)throw std::runtime_error("Cannot start bundled video runtime");
  auto file=utf8(std::filesystem::absolute(path).wstring());media.media=api.mediaNewPath(media.instance,file.c_str());if(!media.media)throw std::runtime_error("Cannot open this local video");
  if(api.parse(media.media,0,10000)==0){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(11);while(!api.parsedStatus(media.media)&&!shared->stopping&&std::chrono::steady_clock::now()<end)Sleep(10);}
  vlc::MediaTrack** metadata=nullptr;const auto trackCount=api.mediaTracks(media.media,&metadata);
  for(unsigned i=0;i<trackCount;++i)if(metadata[i]->type==1&&metadata[i]->video){auto v=metadata[i]->video;pixels.nativeWidth=v->width;pixels.nativeHeight=v->height;if(v->orientation>=4)std::swap(pixels.nativeWidth,pixels.nativeHeight);break;}
  if(metadata)api.freeMediaTracks(metadata,trackCount);if(shared->stopping)return;
  api.mediaOption(media.media,":file-caching=150");
  media.player=api.playerNew(media.media);if(!media.player)throw std::runtime_error("Cannot create local video player");
  api.callbacks(media.player,VlcPixels::lock,VlcPixels::unlock,VlcPixels::display,&pixels);api.formatCallbacks(media.player,VlcPixels::format,nullptr);
  if(api.audioOutput(media.player,"mmdevice")!=0)throw std::runtime_error("Bundled Windows audio output is unavailable");
  auto device=utf8(endpoint);if(!device.empty())api.audioDevice(media.player,nullptr,device.c_str());
  float appliedVolume=-1;{std::lock_guard state(shared->mutex);appliedVolume=shared->wantVolume;api.volume(media.player,int(appliedVolume*100));api.mute(media.player,appliedVolume==0);shared->state.backend=std::string("LibVLC ")+api.version();}
  {std::lock_guard delivery(shared->delivery);shared->accept=true;}
  if(api.play(media.player)!=0)throw std::runtime_error("Cannot play this video with the bundled runtime");
  bool paused=false;double tracksAt=-1,pausedSeekPosition=-1;uint64_t pauseAfterFrame=0;bool pauseAfterSeek=false;
  int pendingAudio=-2,pendingSubtitle=-2,preferredAudio=-2,preferredSubtitle=-2;double pendingSeek=-1;bool restarting=false;
  const auto started=std::chrono::steady_clock::now();
  auto tracks=[&](vlc::TrackDescription* first){std::vector<LocalVideoPlayer::Track> out;for(auto p=first;p;p=p->next)out.push_back({p->id,p->id==-1?"Off":p->name?p->name:"Track "+std::to_string(p->id)});if(first)api.freeTracks(first);return out;};
  while(!shared->stopping){
   bool wantPause,wantLoop;double seek;float volume;int audio,subtitle;
   {std::lock_guard state(shared->mutex);wantPause=shared->wantPause;wantLoop=shared->wantLoop;seek=shared->wantSeek;shared->wantSeek=-1;volume=shared->wantVolume;audio=shared->wantAudioTrack;subtitle=shared->wantSubtitleTrack;shared->wantAudioTrack=shared->wantSubtitleTrack=-2;}
   auto state=api.state(media.player);if(state==7)throw std::runtime_error("Video decoding failed. This codec or file may be unsupported or damaged.");
   if(restarting&&state!=6&&state!=5)restarting=false;
   if(seek>=0)pendingSeek=seek;
   if(volume!=appliedVolume){api.volume(media.player,int(volume*100));api.mute(media.player,volume==0);appliedVolume=volume;}
   if(audio!=-2)preferredAudio=pendingAudio=audio;if(subtitle!=-2)preferredSubtitle=pendingSubtitle=subtitle;
   // Requests can arrive while a file is still opening (e.g. endpoint change).
   // Wait for playback to expose streams instead of silently losing the choice.
   if((state==3||state==4)&&pendingAudio!=-2){auto ok=api.setAudio(media.player,pendingAudio)==0;pendingAudio=-2;std::lock_guard guard(shared->mutex);shared->state.trackStatus=ok?"":"Could not switch audio track";tracksAt=-1;}
   if((state==3||state==4)&&pendingSubtitle!=-2){
    auto ok=api.setSubtitle(media.player,pendingSubtitle)==0;pendingSubtitle=-2;
    // Re-read the current cue after switching. Long cues may have started well
    // before this instant; merely enabling their stream would miss that packet.
    if(ok&&pendingSeek<0&&!pauseAfterSeek)pendingSeek=std::max(0.,api.time(media.player)/1000.);
    std::lock_guard guard(shared->mutex);shared->state.trackStatus=ok?"":"Could not switch subtitle track";tracksAt=-1;
   }
   if(state==6&&wantLoop&&pendingSeek<0){pendingSeek=0;wantPause=false;}
   if((state==6||state==5)&&pendingSeek>=0&&!restarting){
    {std::lock_guard delivery(shared->delivery);shared->accept=false;}
    // New demux input otherwise reselects its default language after a loop.
    {std::lock_guard guard(shared->mutex);if(pendingAudio==-2&&!shared->state.audioTracks.empty())pendingAudio=shared->state.audioTrack;if(pendingSubtitle==-2&&!shared->state.subtitleTracks.empty())pendingSubtitle=shared->state.subtitleTrack;}
    api.stop(media.player);if(api.play(media.player)!=0)throw std::runtime_error("Cannot restart video");
    restarting=true;paused=false;Sleep(5);continue;
   }
   if(pendingSeek>=0&&(state==3||state==4)){
    seek=pendingSeek;pendingSeek=-1;
    {std::lock_guard delivery(shared->delivery);shared->accept=false;}
    ++shared->generation;
    auto duration=api.length(media.player)/1000.;seek=std::clamp(seek,0.,duration>0?std::max(0.,duration-.05):seek);
    if(wantPause){
     // LibVLC 3 vmem supplies no picture PTS. A callback after set_time can
     // still contain the old frame; pausing there leaves the wrong image.
     // Reopen this input at start-time for paused seeks only, so its first
     // decoded picture comes from the requested position. Normal playback
     // retains the inexpensive in-place seek path.
     {std::lock_guard guard(shared->mutex);if(pendingAudio==-2)pendingAudio=preferredAudio!=-2?preferredAudio:shared->state.audioTracks.empty()?-2:shared->state.audioTrack;if(pendingSubtitle==-2)pendingSubtitle=preferredSubtitle!=-2?preferredSubtitle:shared->state.subtitleTracks.empty()?-2:shared->state.subtitleTrack;}
     api.stop(media.player);
     auto replacement=api.mediaNewPath(media.instance,file.c_str());if(!replacement)throw std::runtime_error("Cannot reopen paused video");
     api.mediaOption(replacement,":file-caching=150");auto startTime=std::string(":start-time=")+std::to_string(seek);api.mediaOption(replacement,startTime.c_str());
     api.setMedia(media.player,replacement);api.mediaRelease(media.media);media.media=replacement;
    }else{api.setPause(media.player,0);api.setTime(media.player,int64_t(seek*1000));}
    paused=false;pausedSeekPosition=wantPause?seek:-1;pixels.positionMs=int64_t(seek*1000);
    {std::lock_guard guard(shared->mutex);pauseAfterFrame=shared->state.frames;}
    pauseAfterSeek=wantPause;
    {std::lock_guard delivery(shared->delivery);shared->accept=true;}
    if(wantPause&&api.play(media.player)!=0)throw std::runtime_error("Cannot seek paused video");
   }
   bool soughtFrame=true;if(pauseAfterSeek){std::lock_guard guard(shared->mutex);soughtFrame=shared->state.frames>pauseAfterFrame;}
   if((state==3||state==4)&&wantPause!=paused&&(!pauseAfterSeek||soughtFrame)){
    // vout can redisplay its last image while paused. Stop delivering these
    // callbacks as new source frames (they must not drive FG/OCR or the clock).
    {std::lock_guard delivery(shared->delivery);shared->accept=!wantPause;}
    api.setPause(media.player,wantPause);paused=wantPause;pauseAfterSeek=false;
   }
   if(!wantPause)pausedSeekPosition=-1;
   // VLC may not publish its clock until playback resumes after the first
   // start-time picture. Keep the requested seek time while holding that image.
   const auto pos=paused&&pausedSeekPosition>=0?int64_t(pausedSeekPosition*1000):api.time(media.player);pixels.positionMs=std::max<int64_t>(0,pos);const double seconds=api.length(media.player)/1000.;
   const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
   if(elapsed-tracksAt>.3&&(state==3||state==4)){auto audios=tracks(api.audioTracks(media.player)),subs=tracks(api.subtitleTracks(media.player));auto audioId=api.getAudio(media.player),subtitleId=api.getSubtitle(media.player);auto fps=api.fps(media.player);std::lock_guard guard(shared->mutex);shared->state.audioTracks=std::move(audios);shared->state.subtitleTracks=std::move(subs);shared->state.audioTrack=audioId;shared->state.subtitleTrack=subtitleId;shared->state.audio=std::any_of(shared->state.audioTracks.begin(),shared->state.audioTracks.end(),[](auto& t){return t.id>=0;});shared->state.fps=fps;tracksAt=elapsed;}
   state=api.state(media.player);
   {std::lock_guard guard(shared->mutex);auto& s=shared->state;s.opening=s.frames==0&&state!=6;s.playing=state==3&&!paused;s.paused=paused;s.ended=state==6;s.position=s.ended&&seconds>0?seconds:std::max(0.,pos/1000.);s.duration=std::max(0.,seconds);s.loop=wantLoop;if(elapsed>30&&s.frames==0)throw std::runtime_error("No video frames decoded within 30 seconds");}
   Sleep(5);
  }
  {std::lock_guard delivery(shared->delivery);shared->accept=false;}
 }catch(const std::exception& e){std::lock_guard guard(shared->mutex);shared->state.error=e.what();shared->state.opening=shared->state.playing=false;}
 {std::lock_guard delivery(shared->delivery);shared->accept=false;shared->handler={};}
}
}
struct LocalVideoPlayer::Impl {std::shared_ptr<Shared> shared=std::make_shared<Shared>();std::thread worker;std::filesystem::path runtime;};
LocalVideoPlayer::LocalVideoPlayer(const std::filesystem::path& runtime):impl(std::make_unique<Impl>()){if(!runtime.empty())impl->runtime=runtime;else{wchar_t exe[32768];auto n=GetModuleFileNameW(nullptr,exe,DWORD(std::size(exe)));if(n&&n<std::size(exe))impl->runtime=std::filesystem::path(exe).parent_path()/L"media"/L"vlc";}}
LocalVideoPlayer::~LocalVideoPlayer(){stop();}
void LocalVideoPlayer::stop(){impl->shared->stopping=true;if(impl->worker.joinable())impl->worker.join();impl->shared=std::make_shared<Shared>();}
void LocalVideoPlayer::open(const std::filesystem::path& path,const std::wstring& endpoint,int volume,bool muted,Handler handler){stop();auto s=impl->shared;s->handler=std::move(handler);s->wantVolume=muted?0.f:std::clamp(volume,0,100)/100.f;s->state.opening=true;auto runtime=impl->runtime;impl->worker=std::thread([s,path,endpoint,runtime]{if(std::filesystem::is_regular_file(runtime/L"libvlc.dll"))runVlc(s,path,endpoint,runtime);else run(s,path,endpoint);});}
void LocalVideoPlayer::pause(bool value){std::lock_guard lock(impl->shared->mutex);impl->shared->wantPause=value;}
void LocalVideoPlayer::seek(double value){if(!std::isfinite(value))return;std::lock_guard lock(impl->shared->mutex);impl->shared->wantSeek=std::max(0.,value);}
void LocalVideoPlayer::loop(bool value){std::lock_guard lock(impl->shared->mutex);impl->shared->wantLoop=value;}
void LocalVideoPlayer::volume(int value,bool muted){std::lock_guard lock(impl->shared->mutex);impl->shared->wantVolume=muted?0.f:std::clamp(value,0,100)/100.f;}
void LocalVideoPlayer::selectAudioTrack(int id){std::lock_guard lock(impl->shared->mutex);impl->shared->wantAudioTrack=id;}
void LocalVideoPlayer::selectSubtitleTrack(int id){std::lock_guard lock(impl->shared->mutex);impl->shared->wantSubtitleTrack=id;}
LocalVideoPlayer::Snapshot LocalVideoPlayer::snapshot()const{std::lock_guard lock(impl->shared->mutex);return impl->shared->state;}
std::vector<LocalVideoPlayer::AudioOutput> LocalVideoPlayer::audioOutputs(){
 std::vector<AudioOutput> result{{L"","Windows default output"}};
 ComPtr<IMMDeviceEnumerator> enumerator;if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,IID_PPV_ARGS(&enumerator))))return result;
 ComPtr<IMMDeviceCollection> devices;if(FAILED(enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,&devices)))return result;UINT count=0;devices->GetCount(&count);
 for(UINT i=0;i<count;++i){ComPtr<IMMDevice> device;if(FAILED(devices->Item(i,&device)))continue;LPWSTR id=nullptr;if(FAILED(device->GetId(&id)))continue;std::wstring name=id;
  ComPtr<IPropertyStore> properties;PROPVARIANT value{};if(SUCCEEDED(device->OpenPropertyStore(STGM_READ,&properties))&&SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName,&value))&&value.vt==VT_LPWSTR)name=value.pwszVal;
  result.push_back({id,utf8(name)});PropVariantClear(&value);CoTaskMemFree(id);
 }return result;
}

