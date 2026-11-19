// FrameLab's optional NR component importer. GPL-3.0.
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <thread>
#include "WindowChrome.hpp"
namespace {
HWND statusLabel=nullptr,installButton=nullptr;HFONT font=nullptr;HBRUSH background=nullptr;
bool busy=false;std::filesystem::path app,script;
constexpr UINT completed=WM_APP+1;
const wchar_t* source=L"https://github.com/Likely7/Veyra-NRVideo/releases/download/v2.1.9/Veyra-2.1.9-NVIDIA-win64-portable.zip";
void control(HWND parent,const wchar_t* kind,const wchar_t* value,int id,int y,int height){
 auto w=CreateWindowW(kind,value,WS_CHILD|WS_VISIBLE|(id?WS_TABSTOP:0),24,y,686,height,parent,reinterpret_cast<HMENU>(INT_PTR(id)),nullptr,nullptr);
 SendMessageW(w,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
 if(id==3)installButton=w;if(id==4)statusLabel=w;
}
void import(HWND window){
 wchar_t file[32768]{};OPENFILENAMEW dialog{sizeof(dialog)};dialog.hwndOwner=window;dialog.lpstrFile=file;dialog.nMaxFile=32768;
 dialog.lpstrTitle=L"Vyber Veyra-2.1.9-NVIDIA-win64-portable.zip";dialog.lpstrFilter=L"Veyra NVIDIA portable ZIP\0*.zip\0";dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&dialog))return;
 if(!std::filesystem::is_regular_file(script)){SetWindowTextW(statusLabel,L"Chybí tools/Install-Enhancer.ps1. Rozbal celý FrameLab ZIP.");return;}
 busy=true;EnableWindow(installButton,FALSE);SetWindowTextW(statusLabel,L"Kontroluji ZIP a kopíruji komponenty. Chvíli počkej…");
 std::thread([window,archive=std::wstring(file)]{
  DWORD result=1;
  try{
   wchar_t system[MAX_PATH]{};GetSystemDirectoryW(system,MAX_PATH);
   auto ps=std::filesystem::path(system)/L"WindowsPowerShell/v1.0/powershell.exe";
   std::wstring command=L"\""+ps.wstring()+L"\" -NoProfile -ExecutionPolicy Bypass -File \""+script.wstring()+L"\" -NoGui -AppDir \""+app.wstring()+L"\" -Archive \""+archive+L"\"";
   SECURITY_ATTRIBUTES attributes{sizeof(attributes),nullptr,TRUE};
   auto log=CreateFileW((app/L"nr-import.log").c_str(),GENERIC_WRITE,FILE_SHARE_READ,&attributes,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
   auto input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&attributes,OPEN_EXISTING,0,nullptr);
   STARTUPINFOW start{sizeof(start)};start.dwFlags=STARTF_USESTDHANDLES;start.hStdOutput=start.hStdError=log;start.hStdInput=input;PROCESS_INFORMATION process{};
   if(log!=INVALID_HANDLE_VALUE&&input!=INVALID_HANDLE_VALUE&&CreateProcessW(ps.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,app.parent_path().c_str(),&start,&process)){
    WaitForSingleObject(process.hProcess,INFINITE);GetExitCodeProcess(process.hProcess,&result);CloseHandle(process.hProcess);CloseHandle(process.hThread);
   }
   if(log!=INVALID_HANDLE_VALUE)CloseHandle(log);if(input!=INVALID_HANDLE_VALUE)CloseHandle(input);
  }catch(...){result=1;}
  PostMessageW(window,completed,result,0);
 }).detach();
}
LRESULT CALLBACK windowProc(HWND window,UINT message,WPARAM w,LPARAM l){
 if(message==WM_CREATE){
  font=CreateFontW(-17,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
  control(window,L"STATIC",L"Doplnit enhancer do FrameLabu",0,18,30);
  control(window,L"STATIC",L"1. Stáhni externí ZIP (731 MB). Nerozbaluj ho.\n2. Ukonči FrameLab a vyber ZIP tlačítkem níže.\n3. Znovu spusť FrameLab > AI / NR > Zapnout enhancer.",0,57,81);
  control(window,L"STATIC",L"Komunitní experiment, nikoli oficiální instalace NVIDIA. DLL pocházejí z Veyry; ShortFuse je upravená varianta. Práva k použití a šíření zde nejsou potvrzena. Odkaz ani import licenci neuděluje. Použij s příslušným oprávněním; podrobnosti jsou v návodu.",0,151,112);
  control(window,L"BUTTON",L"1 — Stáhnout externí Veyra 2.1.9 NVIDIA (731 MB)",1,276,43);
  control(window,L"BUTTON",L"Podrobný návod, podmínky a oficiální zdroje",2,329,43);
  control(window,L"BUTTON",L"2 — Vybrat stažený ZIP a doplnit soubory",3,382,43);
  control(window,L"STATIC",L"Frame gen, ukázky a překladač už jsou v hlavním ZIPu. Tento krok doplní pouze NR. Nemění ovladače ani systémové soubory.",4,446,94);return 0;
 }
 if(message==WM_COMMAND){switch(LOWORD(w)){
  case 1:ShellExecuteW(window,L"open",source,nullptr,nullptr,SW_SHOWNORMAL);break;
  case 2:{auto local=app.parent_path()/L"NAVOD ENHANCER.html";ShellExecuteW(window,L"open",std::filesystem::is_regular_file(local)?local.c_str():L"https://opicebot.cz/framelab/guide/enhancer.html",nullptr,nullptr,SW_SHOWNORMAL);break;}
  case 3:if(!busy)import(window);break;
 }return 0;}
 if(message==completed){busy=false;EnableWindow(installButton,TRUE);SetWindowTextW(statusLabel,w==0?L"Soubory jsou připravené. Spusť FrameLab > AI / NR. Začni 65 %, limitem 720p a děleným porovnáním. Podporu GPU ověří inicializace.":L"Import se nepodařil. Zkontroluj správný ZIP a ukonči FrameLab. Podrobná chyba je v app/nr-import.log. Žádná neověřená DLL se neinstaluje.");return 0;}
 if(message==WM_CTLCOLORSTATIC){auto dc=reinterpret_cast<HDC>(w);SetTextColor(dc,RGB(241,243,232));SetBkColor(dc,RGB(16,21,17));return reinterpret_cast<LRESULT>(background);}
 if(message==WM_CLOSE&&busy){MessageBoxW(window,L"Počkej prosím na dokončení kontroly a kopírování.",L"FrameLab",MB_OK);return 0;}
 if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
 return DefWindowProcW(window,message,w,l);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int){
 wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);app=std::filesystem::path(path).parent_path();script=app.parent_path()/L"tools/Install-Enhancer.ps1";
 background=CreateSolidBrush(RGB(16,21,17));WNDCLASSW type{};type.lpfnWndProc=windowProc;type.hInstance=instance;type.lpszClassName=L"FrameLabSetup";type.hbrBackground=background;type.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&type);
 auto window=CreateWindowW(type.lpszClassName,L"FrameLab — doplnit enhancer",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,750,605,nullptr,nullptr,instance,nullptr);
 if(!window)return 1;
 applyWindowChrome(window);ShowWindow(window,SW_SHOWNORMAL);MSG message{};while(GetMessageW(&message,nullptr,0,0)>0){if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
 DeleteObject(font);DeleteObject(background);return 0;
}
