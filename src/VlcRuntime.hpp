#pragma once
#include <Windows.h>
#include <filesystem>
#include <stdexcept>
#include <cstdint>

// Public LibVLC 3.x C ABI, dynamically loaded from our portable runtime only.
// Function signatures: VideoLAN include/vlc/libvlc*.h, tag 3.0.24.
namespace vlc {
struct Instance;struct Media;struct Player;
struct TrackDescription {int id;char* name;TrackDescription* next;};
struct VideoTrack {unsigned height,width,sarNum,sarDen,fpsNum,fpsDen;int orientation,projection;float yaw,pitch,roll,fieldOfView;};
struct MediaTrack {uint32_t codec,originalCodec;int id,type,profile,level;union{void* audio;VideoTrack* video;void* subtitle;};unsigned bitrate;char* language;char* description;};
using Lock=void*(*)(void*,void**);using Unlock=void(*)(void*,void*,void*const*);using Display=void(*)(void*,void*);
using Format=unsigned(*)(void**,char*,unsigned*,unsigned*,unsigned*,unsigned*);using Cleanup=void(*)(void*);
class Runtime {
 HMODULE module=nullptr,core=nullptr;
 template<class T>void get(T& function,const char* name){function=reinterpret_cast<T>(GetProcAddress(module,name));if(!function)throw std::runtime_error(std::string("Missing bundled LibVLC function: ")+name);}
public:
 Instance*(*create)(int,const char*const*);void(*release)(Instance*);
 Media*(*mediaNewPath)(Instance*,const char*);void(*mediaRelease)(Media*);void(*mediaOption)(Media*,const char*);
 int(*parse)(Media*,int,int);int(*parsedStatus)(Media*);unsigned(*mediaTracks)(Media*,MediaTrack***);void(*freeMediaTracks)(MediaTrack**,unsigned);
 Player*(*playerNew)(Media*);void(*playerRelease)(Player*);int(*play)(Player*);void(*stop)(Player*);void(*setPause)(Player*,int);
 void(*setMedia)(Player*,Media*);
 int(*state)(Player*);int64_t(*time)(Player*);void(*setTime)(Player*,int64_t);int64_t(*length)(Player*);float(*fps)(Player*);
 void(*callbacks)(Player*,Lock,Unlock,Display,void*);void(*formatCallbacks)(Player*,Format,Cleanup);
 int(*audioOutput)(Player*,const char*);void(*audioDevice)(Player*,const char*,const char*);int(*volume)(Player*,int);void(*mute)(Player*,int);
 TrackDescription*(*audioTracks)(Player*);TrackDescription*(*subtitleTracks)(Player*);void(*freeTracks)(TrackDescription*);
 int(*getAudio)(Player*);int(*setAudio)(Player*,int);int(*getSubtitle)(Player*);int(*setSubtitle)(Player*,int);
 const char*(*version)();const char*(*error)();
 explicit Runtime(const std::filesystem::path& root){
  // Search the DLL's own directory and system directories, never the cwd/PATH.
  core=LoadLibraryExW((root/L"libvlccore.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  module=LoadLibraryExW((root/L"libvlc.dll").c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!core||!module){if(module)FreeLibrary(module);if(core)FreeLibrary(core);throw std::runtime_error("Bundled video runtime is incomplete. Extract the complete FrameLab package.");}
  try{
#define VLC_BIND(field,name) get(field,"libvlc_" name)
   VLC_BIND(create,"new");VLC_BIND(release,"release");VLC_BIND(mediaNewPath,"media_new_path");VLC_BIND(mediaRelease,"media_release");VLC_BIND(mediaOption,"media_add_option");
   VLC_BIND(parse,"media_parse_with_options");VLC_BIND(parsedStatus,"media_get_parsed_status");VLC_BIND(mediaTracks,"media_tracks_get");VLC_BIND(freeMediaTracks,"media_tracks_release");
   VLC_BIND(playerNew,"media_player_new_from_media");VLC_BIND(playerRelease,"media_player_release");VLC_BIND(play,"media_player_play");VLC_BIND(stop,"media_player_stop");VLC_BIND(setPause,"media_player_set_pause");
   VLC_BIND(setMedia,"media_player_set_media");VLC_BIND(state,"media_player_get_state");VLC_BIND(time,"media_player_get_time");VLC_BIND(setTime,"media_player_set_time");VLC_BIND(length,"media_player_get_length");VLC_BIND(fps,"media_player_get_fps");
   VLC_BIND(callbacks,"video_set_callbacks");VLC_BIND(formatCallbacks,"video_set_format_callbacks");VLC_BIND(audioOutput,"audio_output_set");VLC_BIND(audioDevice,"audio_output_device_set");
   VLC_BIND(volume,"audio_set_volume");VLC_BIND(mute,"audio_set_mute");VLC_BIND(audioTracks,"audio_get_track_description");VLC_BIND(subtitleTracks,"video_get_spu_description");VLC_BIND(freeTracks,"track_description_list_release");
   VLC_BIND(getAudio,"audio_get_track");VLC_BIND(setAudio,"audio_set_track");VLC_BIND(getSubtitle,"video_get_spu");VLC_BIND(setSubtitle,"video_set_spu");VLC_BIND(version,"get_version");VLC_BIND(error,"errmsg");
#undef VLC_BIND
  }catch(...){FreeLibrary(module);FreeLibrary(core);throw;}
 }
 ~Runtime(){if(module)FreeLibrary(module);if(core)FreeLibrary(core);}
 Runtime(const Runtime&)=delete;Runtime& operator=(const Runtime&)=delete;
};
}
