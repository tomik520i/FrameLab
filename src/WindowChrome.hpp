#pragma once
#include <windows.h>
#include <dwmapi.h>

// Keep Windows' native caption, resizing, accessibility and Snap behavior.
// Unsupported DWM attributes fail harmlessly on older Windows versions.
inline void applyWindowChrome(HWND window) {
 HIGHCONTRASTW contrast{sizeof(contrast)};
 const bool highContrast=SystemParametersInfoW(SPI_GETHIGHCONTRAST,sizeof(contrast),&contrast,0)
     && (contrast.dwFlags&HCF_HIGHCONTRASTON);
 const BOOL dark=highContrast?FALSE:TRUE;
 const COLORREF systemDefault=0xffffffff;
 const COLORREF caption=highContrast?systemDefault:RGB(14,19,15);
 const COLORREF text=highContrast?systemDefault:RGB(241,243,232);
 const COLORREF border=highContrast?systemDefault:RGB(47,58,41);
 // Documented DWMWINDOWATTRIBUTE values; also compile with older Windows SDKs.
 DwmSetWindowAttribute(window,20,&dark,sizeof(dark));
 DwmSetWindowAttribute(window,35,&caption,sizeof(caption));
 DwmSetWindowAttribute(window,36,&text,sizeof(text));
 DwmSetWindowAttribute(window,34,&border,sizeof(border));
}
