#pragma once
#include "imgui.h"
#include "FrameLabFonts.hpp"
#include <algorithm>
#include <array>

// Shared visual language with opicebot.cz/framelab. Fonts are embedded so the
// portable executable never needs a download or a system font installation.
namespace ui {
inline ImVec4 rgb(int r,int g,int b,float a=1){return {r/255.f,g/255.f,b/255.f,a};}
inline const ImVec4 background=rgb(16,19,17),panel=rgb(23,28,24),line=rgb(50,59,49);
inline const ImVec4 ink=rgb(243,244,233),muted=rgb(165,175,160),accent=rgb(193,255,94);
inline const ImVec4 orange=rgb(255,129,91),input=rgb(115,193,169);
inline ImFont *body=nullptr,*heading=nullptr,*mono=nullptr,*number=nullptr;
inline std::array<ImFont*,6> subtitleFonts{};
inline ImFont* subtitleFont(float pixels){
 for(auto font:subtitleFonts)if(font&&font->FontSize>=pixels)return font;
 return subtitleFonts.back()?subtitleFonts.back():ImGui::GetFont();
}
inline void initialize(){
 auto& io=ImGui::GetIO();
 static const ImWchar ranges[]={0x20,0x17f,0x2000,0x206f,0};
 ImFontConfig config{};config.FontDataOwnedByAtlas=false;
 auto font=[&](const unsigned char* bytes,int count,float size){return io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(bytes),count,size,&config,ranges);};
 body=font(fonts::sans,sizeof(fonts::sans),17);
 heading=font(fonts::bold,sizeof(fonts::bold),26);
 mono=font(fonts::mono,sizeof(fonts::mono),12);
 number=font(fonts::bold,sizeof(fonts::bold),26);
 // Rasterize subtitles separately from the small UI font. Select a native-size
 // atlas (or downsample) at output resolution, never enlarge the 17px UI glyphs.
 static ImVector<ImWchar> subtitleRanges;
 if(subtitleRanges.empty()){ImFontGlyphRangesBuilder glyphs;glyphs.AddRanges(io.Fonts->GetGlyphRangesDefault());glyphs.AddText("ěščřžýáíéúůďťňĚŠČŘŽÝÁÍÉÚŮĎŤŇ…–—„“”‘’");glyphs.BuildRanges(&subtitleRanges);}
 ImFontConfig subtitleConfig=config;subtitleConfig.OversampleH=1;subtitleConfig.OversampleV=1;
 constexpr float sizes[]={32,48,64,96,144,192};
 for(size_t i=0;i<subtitleFonts.size();++i)subtitleFonts[i]=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(fonts::sans),sizeof(fonts::sans),sizes[i],&subtitleConfig,subtitleRanges.Data);
 io.FontDefault=body;
 ImGui::StyleColorsDark();auto& s=ImGui::GetStyle();
 s.WindowRounding=0;s.WindowBorderSize=0; // NoDecoration alone leaves a 1px frame and clipped edge.
 s.ChildRounding=7;s.ChildBorderSize=0;s.PopupRounding=5;s.PopupBorderSize=1;
 s.FrameRounding=4;s.FrameBorderSize=0;s.GrabRounding=3;s.TabRounding=3;
 s.ScrollbarSize=9;s.ScrollbarRounding=6;s.WindowPadding={20,18};
 s.ItemSpacing={10,8};s.ItemInnerSpacing={8,6};s.FramePadding={12,7};s.CellPadding={8,6};
 auto& c=s.Colors;
 c[ImGuiCol_Text]=ink;c[ImGuiCol_TextDisabled]=muted;c[ImGuiCol_WindowBg]=background;
 c[ImGuiCol_ChildBg]=panel;c[ImGuiCol_PopupBg]=panel;c[ImGuiCol_Border]=line;c[ImGuiCol_BorderShadow]={0,0,0,0};
 c[ImGuiCol_FrameBg]=rgb(32,39,33);c[ImGuiCol_FrameBgHovered]=rgb(43,54,39);c[ImGuiCol_FrameBgActive]=rgb(57,73,44);
 c[ImGuiCol_Button]=rgb(39,48,37);c[ImGuiCol_ButtonHovered]=rgb(57,73,44);c[ImGuiCol_ButtonActive]=rgb(75,95,53);
 c[ImGuiCol_CheckMark]=accent;c[ImGuiCol_SliderGrab]=accent;c[ImGuiCol_SliderGrabActive]=rgb(218,255,153);
 c[ImGuiCol_Header]=rgb(43,54,37);c[ImGuiCol_HeaderHovered]=rgb(54,70,42);c[ImGuiCol_HeaderActive]=rgb(62,80,43);
 c[ImGuiCol_Separator]=line;c[ImGuiCol_SeparatorHovered]=accent;c[ImGuiCol_SeparatorActive]=accent;
 c[ImGuiCol_ScrollbarBg]=background;c[ImGuiCol_ScrollbarGrab]=line;c[ImGuiCol_ScrollbarGrabHovered]=muted;c[ImGuiCol_ScrollbarGrabActive]=accent;
 c[ImGuiCol_Tab]=panel;c[ImGuiCol_TabHovered]=rgb(49,64,36);c[ImGuiCol_TabSelected]=rgb(43,56,33);c[ImGuiCol_TabSelectedOverline]=accent;
 c[ImGuiCol_TabDimmed]=panel;c[ImGuiCol_TabDimmedSelected]=rgb(37,45,32);c[ImGuiCol_TabDimmedSelectedOverline]=line;
 c[ImGuiCol_TableHeaderBg]=panel;c[ImGuiCol_TableBorderStrong]=line;c[ImGuiCol_TableBorderLight]=line;
 c[ImGuiCol_TableRowBg]={0,0,0,0};c[ImGuiCol_TableRowBgAlt]=rgb(193,255,94,.025f);
 c[ImGuiCol_TextSelectedBg]=rgb(193,255,94,.25f);c[ImGuiCol_NavCursor]=accent;
 c[ImGuiCol_ResizeGrip]=rgb(193,255,94,.2f);c[ImGuiCol_ResizeGripHovered]=rgb(193,255,94,.5f);c[ImGuiCol_ResizeGripActive]=accent;
}
inline void eyebrow(const char* text){ImGui::PushFont(mono);ImGui::TextColored(muted,"%s",text);ImGui::PopFont();}
inline void title(const char* text){ImGui::PushFont(heading);ImGui::TextUnformatted(text);ImGui::PopFont();}
inline void field(const char* label){ImGui::TextColored(muted,"%s",label);ImGui::SetNextItemWidth(-1);}
inline bool button(const char* label,bool primary=false,ImVec2 size={0,0}){
 if(primary){ImGui::PushStyleColor(ImGuiCol_Button,accent);ImGui::PushStyleColor(ImGuiCol_ButtonHovered,rgb(212,255,147));ImGui::PushStyleColor(ImGuiCol_ButtonActive,rgb(168,227,78));ImGui::PushStyleColor(ImGuiCol_Text,background);}
 bool clicked=ImGui::Button(label,size);if(primary)ImGui::PopStyleColor(4);return clicked;
}
inline void nextButton(const char* label){
 float width=ImGui::CalcTextSize(label).x+2*ImGui::GetStyle().FramePadding.x;
 if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+width<=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x)ImGui::SameLine();
}
inline void brand(){
 ImGui::PushFont(heading);ImGui::TextUnformatted("FRAME");ImGui::SameLine(0,1);
 ImGui::TextColored(accent,"/");ImGui::SameLine(0,1);ImGui::TextUnformatted("LAB");ImGui::PopFont();
}
inline void section(const char* label){ImGui::Spacing();ImGui::Separator();ImGui::Spacing();eyebrow(label);}
}
