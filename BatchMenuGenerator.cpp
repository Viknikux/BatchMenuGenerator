// BatchMenuGenerator.cpp - Standalone native Win32 application
// Pure Win32 API, zero external runtime dependencies
// Statically link MSVCRT for out-of-the-box operation on all Windows versions

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>

#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")

#define IDC_EDIT_TITLE          100
#define IDC_EDIT_NUMOPTIONS     101
#define IDC_UPDOWN_OPTIONS      102
#define IDC_BTN_APPLY           103
#define IDC_BTN_THEME           104
#define IDC_BTN_GENERATE        105
#define IDC_PREVIEW             106
#define IDC_SCROLLCONTAINER     107

#define IDC_OPTION_GROUP_FIRST  200
#define IDC_OPTION_EDIT_LABEL   300
#define IDC_OPTION_EDIT_CMD     400
#define IDC_OPTION_COMBO_MODE   500

enum class ThemeMode { Light, Dark };

struct OptionData {
    std::wstring label;
    std::wstring command;
    int runMode;
};

static struct {
    HINSTANCE   hInst;
    HWND        hWnd;
    HWND        hEditTitle;
    HWND        hEditNumOptions;
    HWND        hUpDown;
    HWND        hBtnApply;
    HWND        hBtnGenerate;
    HWND        hPreview;
    HWND        hScrollContainer;
    HWND        hwndThemeToggle;

    ThemeMode   theme;
    HBRUSH      hbrBg;
    HBRUSH      hbrEditBgDark;
    HBRUSH      hbrBtnBgDark;
    HPEN        hpenGroup;
    COLORREF    crText;
    COLORREF    crEditText;
    COLORREF    crBtnText;

    std::vector<OptionData> options;
    int         optionCount;
    int         scrollPos;
    HFONT       hFont;
    HFONT       hFixedFont;
} g = {};

// ---------------------------------------------------------------------------
// Crash Diagnostics Log Engine
// ---------------------------------------------------------------------------
LONG WINAPI CrashDiagnosticsFilter(EXCEPTION_POINTERS* pExceptionInfo) {
    wchar_t logMessage[512];
    swprintf(logMessage, 512,
        L"CRASH LOG INTERCEPTED!\n\n"
        L"Exception Code: 0x%08X\n"
        L"Fault Address:  0x%p\n\n"
        L"The system caught a memory violation at this instruction location.",
        pExceptionInfo->ExceptionRecord->ExceptionCode,
        pExceptionInfo->ExceptionRecord->ExceptionAddress);
    
    MessageBoxW(NULL, logMessage, L"Runtime Crash Log Diagnostics", MB_OK | MB_ICONERROR | MB_TASKMODAL);
    return EXCEPTION_EXECUTE_HANDLER;
}

// ---------------------------------------------------------------------------
// Theme Engine
// ---------------------------------------------------------------------------
static void InitThemeBrushes() {
    if (!g.hbrBg) {
        if (g.theme == ThemeMode::Dark) {
            g.hbrBg        = CreateSolidBrush(RGB(32,32,32));
            g.hbrEditBgDark= CreateSolidBrush(RGB(24,24,24));
            g.hbrBtnBgDark = CreateSolidBrush(RGB(45,45,45));
            g.hpenGroup    = CreatePen(PS_SOLID,1,RGB(70,70,70));
            g.crText       = RGB(220,220,220);
            g.crEditText   = RGB(220,220,220);
            g.crBtnText    = RGB(220,220,220);
        } else {
            g.hbrBg        = CreateSolidBrush(RGB(245,245,245));
            g.hbrEditBgDark= CreateSolidBrush(RGB(255,255,255));
            g.hbrBtnBgDark = CreateSolidBrush(RGB(225,225,225));
            g.hpenGroup    = CreatePen(PS_SOLID,1,RGB(200,200,200));
            g.crText       = RGB(0,0,0);
            g.crEditText   = RGB(0,0,0);
            g.crBtnText    = RGB(0,0,0);
        }
    }
}

static void DestroyThemeBrushes() {
    if (g.hbrBg)         { DeleteObject(g.hbrBg);        g.hbrBg = nullptr; }
    if (g.hbrEditBgDark){ DeleteObject(g.hbrEditBgDark);g.hbrEditBgDark=nullptr; }
    if (g.hbrBtnBgDark) { DeleteObject(g.hbrBtnBgDark); g.hbrBtnBgDark=nullptr; }
    if (g.hpenGroup)    { DeleteObject(g.hpenGroup);    g.hpenGroup=nullptr; }
}

static ThemeMode DetectSystemTheme() {
    HKEY hKey; DWORD v=1,sz=sizeof(v);
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0,KEY_READ,&hKey)==ERROR_SUCCESS) {
        RegQueryValueExW(hKey,L"AppsUseLightTheme",NULL,NULL,(LPBYTE)&v,&sz);
        RegCloseKey(hKey);
    }
    return v?ThemeMode::Light:ThemeMode::Dark;
}

static void ApplyThemeToWindow(HWND hwnd) {
    DestroyThemeBrushes();
    InitThemeBrushes();

    HMODULE hDwm=LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        typedef HRESULT(WINAPI *F)(HWND,DWORD,LPCVOID,DWORD);
        F p=(F)GetProcAddress(hDwm,"DwmSetWindowAttribute");
        if (p) { BOOL v=(g.theme==ThemeMode::Dark)?TRUE:FALSE; p(hwnd,20,&v,sizeof(v)); }
        FreeLibrary(hDwm);
    }
    if (g.hwndThemeToggle)
        SetWindowTextW(g.hwndThemeToggle,
            g.theme==ThemeMode::Dark?L"\u263D":L"\u2600");

    InvalidateRect(hwnd,NULL,TRUE);
    RedrawWindow(hwnd,NULL,NULL,RDW_ERASE|RDW_INVALIDATE|RDW_ALLCHILDREN);
}

// ---------------------------------------------------------------------------
// Batch Engine
// ---------------------------------------------------------------------------
static std::wstring EscapeBatch(const std::wstring& s,bool c) {
    std::wstring r; r.reserve(s.length()*2);
    for (auto ch : s) {
        switch (ch) {
            case L'%':r+=L"%%";break; case L'^':r+=L"^^";break;
            case L'&':r+=L"^&";break; case L'<':r+=L"^<";break;
            case L'>':r+=L"^>";break; case L'|':r+=L"^|";break;
            case L'"':r+=c?L"\"\"":L"^\"";break;
            default:r+=ch;break;
        }
    }
    return r;
}

static std::wstring GenerateBatch() {
    wchar_t b[256]={0};
    if (g.hEditTitle) GetWindowTextW(g.hEditTitle,b,256);
    std::wstring t=b[0]?b:L"Batch Menu";
    std::wstring bat; bat.reserve(4096);
    bat+=L"Created with BatchMenuGenerator!\r\n";
    bat+=L"@echo off\r\ntitle "+t+L"\r\n:menu\r\ncls\r\n";
    bat+=L"echo =========================================================\r\n";
    bat+=L"echo                        "+t+L"\r\n";
    bat+=L"echo =========================================================\r\necho.\r\n";

    for (int i=0;i<g.optionCount&&i<(int)g.options.size();i++) {
        std::wstring l=g.options[i].label.empty()?L"Option":g.options[i].label;
        swprintf(b,256,L"echo  %d) %s\r\n",i+1,l.c_str()); bat+=b;
    }
    bat+=L"echo  X) Exit\r\necho.\r\n";
    bat+=L"echo =========================================================\r\necho.\r\n";

    swprintf(b,256,L"set /p choice=\"Select Option (1-%d): \"\r\n",g.optionCount);
    bat+=b;
    for (int i=0;i<g.optionCount;i++) {
        swprintf(b,256,L"if \"%%choice%%\"==\"%d\" goto run_option%d\r\n",i+1,i+1);
        bat+=b;
    }
    bat+=L"if \"%choice%\"==\"X\" goto close_menu\r\n";
    bat+=L"if \"%choice%\"==\"x\" goto close_menu\r\ngoto menu\r\n\r\n";

    for (int i=0;i<g.optionCount;i++) {
        swprintf(b,256,L":run_option%d\r\n",i+1); bat+=b; bat+=L"cls\r\n";
        std::wstring cmd=g.options[i].command;
        if (cmd.length()>=2&&cmd[0]==L'"'&&cmd.back()==L'"')
            cmd=cmd.substr(1,cmd.length()-2);
        if (cmd.empty()) bat+=L"echo No command specified.\r\n";
        else if (g.options[i].runMode==1)
            bat+=L"cmd /c \""+EscapeBatch(cmd,true)+L"\"\r\n";
        else
            bat+=L"call "+EscapeBatch(cmd,false)+L"\r\n";
        bat+=L"echo.\r\npause\r\ngoto menu\r\n\r\n";
    }
    bat+=L":close_menu\r\ncls\r\ntitle cmd\r\ngoto :eof\r\n";
    return bat;
}

static void UpdatePreview() {
    if (g.hPreview) SetWindowTextW(g.hPreview,GenerateBatch().c_str());
}

// ---------------------------------------------------------------------------
// Option Control Management
// ---------------------------------------------------------------------------
static void DestroyOptionControls() {
    if (g.hScrollContainer) {
        HWND c=GetWindow(g.hScrollContainer,GW_CHILD);
        while (c) { HWND n=GetWindow(c,GW_HWNDNEXT); DestroyWindow(c); c=n; }
    }
    g.optionCount=0; g.options.clear();
}

static void ReadOptions() {
    if (!g.hScrollContainer) return;
    for (int i=0;i<g.optionCount&&i<(int)g.options.size();i++) {
        wchar_t buf[1024]; HWND h;
        h=GetDlgItem(g.hScrollContainer,IDC_OPTION_EDIT_LABEL+i);
        if (h) { GetWindowTextW(h,buf,1024); g.options[i].label=buf; }
        h=GetDlgItem(g.hScrollContainer,IDC_OPTION_EDIT_CMD+i);
        if (h) { GetWindowTextW(h,buf,1024); g.options[i].command=buf; }
        h=GetDlgItem(g.hScrollContainer,IDC_OPTION_COMBO_MODE+i);
        if (h) { g.options[i].runMode=(int)SendMessageW(h,CB_GETCURSEL,0,0); }
    }
}

static void CreateOptions(int count) {
    if (!g.hScrollContainer||count<=0) return;
    DestroyOptionControls();
    g.options.resize(count); g.optionCount=count;

    RECT rc; GetClientRect(g.hScrollContainer,&rc);
    int cx=max(16,rc.right-rc.left), GH=98, GS=6;

    if (!g.hbrBg) ApplyThemeToWindow(g.hWnd);

    for (int i=0;i<count;i++) {
        int y=i*(GH+GS); wchar_t lb[32];
        swprintf(lb,32,L" Option %d ",i+1);
        HWND h;

        h=CreateWindowW(L"BUTTON",lb,WS_CHILD|WS_VISIBLE|BS_GROUPBOX,
            8,y,cx-16,GH,g.hScrollContainer,
            (HMENU)(INT_PTR)(IDC_OPTION_GROUP_FIRST+i),g.hInst,NULL);

        CreateWindowW(L"STATIC",L"Label:",WS_CHILD|WS_VISIBLE|SS_RIGHT,
            16,y+18,48,20,g.hScrollContainer,NULL,g.hInst,NULL);

        h=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|
            ES_LEFT|ES_AUTOHSCROLL,68,y+17,cx-92,22,g.hScrollContainer,
            (HMENU)(INT_PTR)(IDC_OPTION_EDIT_LABEL+i),g.hInst,NULL);

        CreateWindowW(L"STATIC",L"Command:",WS_CHILD|WS_VISIBLE|SS_RIGHT,
            16,y+44,56,20,g.hScrollContainer,NULL,g.hInst,NULL);

        h=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|
            ES_LEFT|ES_AUTOHSCROLL,76,y+43,cx-100,22,g.hScrollContainer,
            (HMENU)(INT_PTR)(IDC_OPTION_EDIT_CMD+i),g.hInst,NULL);

        CreateWindowW(L"STATIC",L"Mode:",WS_CHILD|WS_VISIBLE|SS_RIGHT,
            16,y+70,40,20,g.hScrollContainer,NULL,g.hInst,NULL);

        h=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|
            CBS_DROPDOWNLIST|CBS_HASSTRINGS,60,y+69,cx-84,120,
            g.hScrollContainer,(HMENU)(INT_PTR)(IDC_OPTION_COMBO_MODE+i),
            g.hInst,NULL);
        SendMessageW(h,CB_ADDSTRING,0,(LPARAM)L"Direct command");
        SendMessageW(h,CB_ADDSTRING,0,(LPARAM)L"cmd /c wrapper");
        SendMessageW(h,CB_SETCURSEL,0,0);
    }

    int th=count*(GH+GS)+8;
    SCROLLINFO si={sizeof(si),SIF_ALL};
    GetScrollInfo(g.hScrollContainer,SB_VERT,&si);
    si.nMin=0; si.nMax=th; si.nPage=rc.bottom-rc.top;
    if (si.nPage>(UINT)th) si.nPage=th;
    si.nPos=0; si.fMask=SIF_ALL;
    SetScrollInfo(g.hScrollContainer,SB_VERT,&si,TRUE);
    g.scrollPos=0;

    InvalidateRect(g.hScrollContainer,NULL,TRUE);
    UpdatePreview();
}

// ---------------------------------------------------------------------------
// Scroll Container Window Procedure
// ---------------------------------------------------------------------------
static LRESULT CALLBACK ScrollProc(HWND h,UINT m,WPARAM w,LPARAM l) {
    switch (m) {
    case WM_VSCROLL: {
        SCROLLINFO si={sizeof(si),SIF_ALL};
        GetScrollInfo(h,SB_VERT,&si); int op=si.nPos;
        switch (LOWORD(w)) {
        case SB_TOP:       si.nPos=si.nMin; break;
        case SB_BOTTOM:    si.nPos=si.nMax; break;
        case SB_LINEUP:    si.nPos-=12; break;
        case SB_LINEDOWN:  si.nPos+=12; break;
        case SB_PAGEUP:    si.nPos-=(int)max(1,(int)si.nPage); break;
        case SB_PAGEDOWN:  si.nPos+=(int)max(1,(int)si.nPage); break;
        case SB_THUMBTRACK: case SB_THUMBPOSITION: si.nPos=si.nTrackPos; break;
        }
        int ms=max(0,si.nMax-(int)max(1,si.nPage));
        si.nPos=max(si.nMin,min(si.nPos,ms));
        if (si.nPos!=op) {
            ScrollWindowEx(h,0,op-si.nPos,NULL,NULL,NULL,NULL,
                SW_SCROLLCHILDREN|SW_INVALIDATE|SW_ERASE);
            SetScrollInfo(h,SB_VERT,&si,TRUE); g.scrollPos=si.nPos;
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {
        UINT sl=3; SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0,&sl,0);
        if (!sl) sl=1;
        int d=-(GET_WHEEL_DELTA_WPARAM(w)/WHEEL_DELTA)*(int)sl*12;
        SCROLLINFO si={sizeof(si),SIF_ALL};
        GetScrollInfo(h,SB_VERT,&si); int op=si.nPos;
        si.nPos+=d; int ms=max(0,si.nMax-(int)max(1,si.nPage));
        si.nPos=max(si.nMin,min(si.nPos,ms));
        if (si.nPos!=op) {
            ScrollWindowEx(h,0,op-si.nPos,NULL,NULL,NULL,NULL,
                SW_SCROLLCHILDREN|SW_INVALIDATE|SW_ERASE);
            SetScrollInfo(h,SB_VERT,&si,TRUE); g.scrollPos=si.nPos;
        }
        return 0;
    }
    case WM_SIZE: {
        int wc=LOWORD(l),hc=HIWORD(l);
        if (wc<=0||hc<=0) break;
        int GH=98,GS=6;
        for (int i=0;i<g.optionCount;i++) {
            int by=i*(GH+GS); HWND hw;
            hw=GetDlgItem(h,IDC_OPTION_GROUP_FIRST+i);
            if (hw) SetWindowPos(hw,NULL,8,by,wc-16,GH,SWP_NOZORDER);
            hw=GetDlgItem(h,IDC_OPTION_EDIT_LABEL+i);
            if (hw) SetWindowPos(hw,NULL,68,by+17,wc-92,22,SWP_NOZORDER);
            hw=GetDlgItem(h,IDC_OPTION_EDIT_CMD+i);
            if (hw) SetWindowPos(hw,NULL,76,by+43,wc-100,22,SWP_NOZORDER);
            hw=GetDlgItem(h,IDC_OPTION_COMBO_MODE+i);
            if (hw) SetWindowPos(hw,NULL,60,by+69,wc-84,120,SWP_NOZORDER);
        }
        int th=g.optionCount*(GH+GS)+8;
        SCROLLINFO si={sizeof(si),SIF_ALL};
        GetScrollInfo(h,SB_VERT,&si);
        si.nMin=0; si.nMax=th; si.nPage=hc;
        int ms=max(0,th-(int)si.nPage); if (si.nPos>ms) si.nPos=ms;
        si.fMask=SIF_ALL; SetScrollInfo(h,SB_VERT,&si,TRUE);
        g.scrollPos=si.nPos;
        return DefWindowProcW(h,m,w,l);
    }
    case WM_CTLCOLORSTATIC: {
        SetTextColor((HDC)w,g.crText);
        SetBkColor((HDC)w,g.theme==ThemeMode::Dark?RGB(32,32,32):RGB(245,245,245));
        return (LRESULT)(g.hbrBg?g.hbrBg:GetStockObject(WHITE_BRUSH));
    }
    case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX: {
        SetTextColor((HDC)w,g.crEditText);
        SetBkColor((HDC)w,g.theme==ThemeMode::Dark?RGB(24,24,24):RGB(255,255,255));
        return (LRESULT)(g.hbrEditBgDark?g.hbrEditBgDark:GetStockObject(WHITE_BRUSH));
    }
    case WM_ERASEBKGND:
        if (g.theme==ThemeMode::Dark) {
            RECT rc; GetClientRect(h,&rc);
            if (g.hbrBg) FillRect((HDC)w,&rc,g.hbrBg);
            return 1;
        }
        return DefWindowProcW(h,m,w,l);
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(h,&ps);
        if (g.theme==ThemeMode::Dark) {
            RECT rc; GetClientRect(h,&rc);
            FillRect(dc,&rc,g.hbrBg?g.hbrBg:(HBRUSH)GetStockObject(WHITE_BRUSH));
        }
        EndPaint(h,&ps);
        return 0;
    }
    }
    return DefWindowProcW(h,m,w,l);
}

// ---------------------------------------------------------------------------
// Main Window Procedure
// ---------------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        g.hWnd=h;
        g.theme=DetectSystemTheme();
        InitThemeBrushes();

        NONCLIENTMETRICSW nm={sizeof(nm)};
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(nm),&nm,0);
        g.hFont=CreateFontIndirectW(&nm.lfMessageFont);

        g.hwndThemeToggle=CreateWindowW(L"BUTTON",L"\u2600",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
            760,8,30,24,h,(HMENU)(INT_PTR)IDC_BTN_THEME,g.hInst,NULL);
        if (g.hFont) SendMessageW(g.hwndThemeToggle,WM_SETFONT,(WPARAM)g.hFont,0);

        CreateWindowW(L"STATIC",L"Window Title:",WS_CHILD|WS_VISIBLE,
            10,10,88,22,h,NULL,g.hInst,NULL);
        g.hEditTitle=CreateWindowW(L"EDIT",L"Batch Menu",
            WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_AUTOHSCROLL,
            102,9,300,24,h,(HMENU)(INT_PTR)IDC_EDIT_TITLE,g.hInst,NULL);
        if (g.hFont) SendMessageW(g.hEditTitle,WM_SETFONT,(WPARAM)g.hFont,0);

        CreateWindowW(L"STATIC",L"Number of Options:",WS_CHILD|WS_VISIBLE,
            10,40,108,22,h,NULL,g.hInst,NULL);
        g.hEditNumOptions=CreateWindowW(L"EDIT",L"3",
            WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_NUMBER|ES_AUTOHSCROLL,
            122,39,40,24,h,(HMENU)(INT_PTR)IDC_EDIT_NUMOPTIONS,g.hInst,NULL);
        if (g.hFont) SendMessageW(g.hEditNumOptions,WM_SETFONT,(WPARAM)g.hFont,0);

        g.hUpDown=CreateWindowW(UPDOWN_CLASSW,NULL,
            WS_CHILD|WS_VISIBLE|UDS_SETBUDDYINT|UDS_ALIGNRIGHT|
            UDS_AUTOBUDDY|UDS_ARROWKEYS|UDS_NOTHOUSANDS,
            162,39,0,0,h,(HMENU)(INT_PTR)IDC_UPDOWN_OPTIONS,g.hInst,NULL);
        SendMessageW(g.hUpDown,UDM_SETRANGE,0,MAKELPARAM(20,1));
        SendMessageW(g.hUpDown,UDM_SETBUDDY,(WPARAM)g.hEditNumOptions,0);
        SendMessageW(g.hUpDown,UDM_SETPOS,0,MAKELPARAM(3,0));

        g.hBtnApply=CreateWindowW(L"BUTTON",L"Apply",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
            210,39,60,24,h,(HMENU)(INT_PTR)IDC_BTN_APPLY,g.hInst,NULL);
        if (g.hFont) SendMessageW(g.hBtnApply,WM_SETFONT,(WPARAM)g.hFont,0);

        g.hScrollContainer=CreateWindowW(L"STATIC",NULL,
            WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_CLIPCHILDREN|SS_NOTIFY,
            0,70,800,200,h,(HMENU)(INT_PTR)IDC_SCROLLCONTAINER,g.hInst,NULL);
        SetWindowLongPtrW(g.hScrollContainer,GWLP_WNDPROC,(LONG_PTR)ScrollProc);

        g.hBtnGenerate=CreateWindowW(L"BUTTON",L"Generate Script",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
            10,280,120,32,h,(HMENU)(INT_PTR)IDC_BTN_GENERATE,g.hInst,NULL);
        if (g.hFont) SendMessageW(g.hBtnGenerate,WM_SETFONT,(WPARAM)g.hFont,0);

        g.hPreview=CreateWindowW(L"EDIT",L"",
            WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL|WS_VSCROLL|WS_HSCROLL|ES_READONLY,
            10,320,780,160,h,(HMENU)(INT_PTR)IDC_PREVIEW,g.hInst,NULL);
        
        g.hFixedFont=CreateFontW(14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH|FF_MODERN,L"Consolas");
        if (g.hFixedFont) SendMessageW(g.hPreview,WM_SETFONT,(WPARAM)g.hFixedFont,0);

        CreateOptions(3);
        return 0;
    }

    case WM_SIZE: {
        int W=LOWORD(l),H=HIWORD(l);
        int pH=160, bY=H-pH-40, cb=max(70,bY-5);
        SetWindowPos(g.hwndThemeToggle,NULL,max(760,W-50),8,30,24,SWP_NOZORDER);
        SetWindowPos(g.hEditTitle,NULL,102,9,min(300,W-200),24,SWP_NOZORDER);
        SetWindowPos(g.hUpDown,NULL,162,39,0,0,SWP_NOZORDER|SWP_NOSIZE);
        SetWindowPos(g.hScrollContainer,NULL,0,70,W,max(1,cb-70),SWP_NOZORDER);
        SetWindowPos(g.hBtnGenerate,NULL,10,bY,120,32,SWP_NOZORDER);
        SetWindowPos(g.hPreview,NULL,10,bY+38,max(1,W-20),max(1,pH-2),SWP_NOZORDER);
        if (g.hScrollContainer) {
            RECT rc; GetClientRect(g.hScrollContainer,&rc);
            SendMessageW(g.hScrollContainer,WM_SIZE,0,
                MAKELPARAM(rc.right-rc.left,rc.bottom-rc.top));
        }
        return DefWindowProcW(h,m,w,l);
    }

    case WM_COMMAND: {
        WORD id=LOWORD(w),c=HIWORD(w);
        if ((id==IDC_BTN_APPLY&&c==BN_CLICKED)||
            (id==IDC_EDIT_NUMOPTIONS&&c==EN_UPDATE)) {
            if (g.hEditNumOptions && g.hScrollContainer) {
                wchar_t b[16]; GetWindowTextW(g.hEditNumOptions,b,16);
                int n=_wtoi(b); if (n<1) n=1; if (n>20) n=20;
                ReadOptions(); CreateOptions(n);
            }
            return 0;
        }
        if (id==IDC_BTN_THEME&&c==BN_CLICKED) {
            g.theme=(g.theme==ThemeMode::Dark)?ThemeMode::Light:ThemeMode::Dark;
            ApplyThemeToWindow(h);
            if (g.hScrollContainer) {
                InvalidateRect(g.hScrollContainer,NULL,TRUE);
                RedrawWindow(g.hScrollContainer,NULL,NULL,
                    RDW_ERASE|RDW_INVALIDATE|RDW_ALLCHILDREN);
            }
            return 0;
        }
        if (id==IDC_BTN_GENERATE&&c==BN_CLICKED) {
            ReadOptions();
            std::wstring ct=GenerateBatch();
            SetWindowTextW(g.hPreview,ct.c_str());
            wchar_t p[MAX_PATH]={0};
            OPENFILENAMEW of={sizeof(of)};
            of.hwndOwner=h; of.lpstrFilter=L"Batch Files\0*.bat\0All Files\0*.*\0";
            of.lpstrFile=p; of.nMaxFile=MAX_PATH; of.lpstrDefExt=L"bat";
            of.lpstrTitle=L"Save Batch Menu Script";
            of.Flags=OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;
            if (GetSaveFileNameW(&of)) {
                HANDLE hf=CreateFileW(p,GENERIC_WRITE,0,NULL,
                    CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
                if (hf!=INVALID_HANDLE_VALUE) {
                    DWORD wr; BYTE bom[]={0xEF,0xBB,0xBF};
                    WriteFile(hf,bom,3,&wr,NULL);
                    int len=WideCharToMultiByte(CP_UTF8,0,ct.c_str(),
                        (int)ct.length(),NULL,0,NULL,NULL);
                    if (len>0) {
                        std::vector<char> u(len);
                        WideCharToMultiByte(CP_UTF8,0,ct.c_str(),
                            (int)ct.length(),u.data(),len,NULL,NULL);
                        WriteFile(hf,u.data(),len,&wr,NULL);
                    }
                    CloseHandle(hf);
                    wchar_t msg[512]; swprintf(msg,512,
                        L"Batch menu created!\n\nSaved to:\n%s",p);
                    MessageBoxW(h,msg,L"Success",MB_OK|MB_ICONINFORMATION);
                } else {
                    MessageBoxW(h,L"Failed to write file.",L"Error",MB_OK|MB_ICONERROR);
                }
            }
            return 0;
        }
        if (c==EN_UPDATE&&id==IDC_EDIT_TITLE) UpdatePreview();
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        SetTextColor((HDC)w,g.crText);
        SetBkColor((HDC)w,g.theme==ThemeMode::Dark?RGB(32,32,32):RGB(245,245,245));
        return (LRESULT)(g.hbrBg?g.hbrBg:GetStockObject(WHITE_BRUSH));
    }
    case WM_CTLCOLOREDIT: {
        HWND hE=(HWND)l;
        if (hE==g.hEditTitle||hE==g.hEditNumOptions||hE==g.hPreview) {
            SetTextColor((HDC)w,g.crEditText);
            SetBkColor((HDC)w,g.theme==ThemeMode::Dark?RGB(24,24,24):RGB(255,255,255));
            return (LRESULT)(g.hbrEditBgDark?g.hbrEditBgDark:GetStockObject(WHITE_BRUSH));
        }
        return DefWindowProcW(h,m,w,l);
    }
    case WM_CTLCOLORLISTBOX: {
        SetTextColor((HDC)w,g.crEditText);
        SetBkColor((HDC)w,g.theme==ThemeMode::Dark?RGB(24,24,24):RGB(255,255,255));
        return (LRESULT)(g.hbrEditBgDark?g.hbrEditBgDark:GetStockObject(WHITE_BRUSH));
    }
    case WM_CTLCOLORBTN: {
        HWND hB=(HWND)l;
        if (hB==g.hwndThemeToggle||hB==g.hBtnApply||hB==g.hBtnGenerate) {
            SetTextColor((HDC)w,g.crBtnText);
            SetBkColor((HDC)w,g.theme==ThemeMode::Dark?RGB(45,45,45):RGB(225,225,225));
            return (LRESULT)(g.hbrBtnBgDark?g.hbrBtnBgDark:GetStockObject(LTGRAY_BRUSH));
        }
        return DefWindowProcW(h,m,w,l);
    }
    case WM_ERASEBKGND:
        if (g.theme==ThemeMode::Dark) {
            RECT rc; GetClientRect(h,&rc);
            if (g.hbrBg) FillRect((HDC)w,&rc,g.hbrBg);
            return 1;
        }
        return DefWindowProcW(h,m,w,l);
    case WM_DESTROY: 
        DestroyThemeBrushes(); 
        if (g.hFont) DeleteObject(g.hFont);
        if (g.hFixedFont) DeleteObject(g.hFixedFont);
        PostQuitMessage(0); 
        return 0;
    case WM_CLOSE:   DestroyWindow(h); return 0;
    }
    return DefWindowProcW(h,m,w,l);
}

// ---------------------------------------------------------------------------
// Entry Point
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nC) {
    // Intercept hardware faults and display code addresses instead of crashing silently
    SetUnhandledExceptionFilter(CrashDiagnosticsFilter);

    g.hInst = hI;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    const wchar_t CN[] = L"BatchMenuGeneratorClass";
        
    // Clean unified window configuration utilizing compiled icon asset tables (ID 1)
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hI;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CN;
    wc.hIcon = LoadIconW(hI, MAKEINTRESOURCEW(1));   // Displayed in Taskbar & Alt+Tab
    wc.hIconSm = LoadIconW(hI, MAKEINTRESOURCEW(1)); // Displayed in Window Top-Left Titlebar

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    int W = 820, H = 720, sW = GetSystemMetrics(SM_CXSCREEN), sH = GetSystemMetrics(SM_CYSCREEN);
    int x = (sW - W) / 2, y = max(0, (sH - H) / 2 - 40);

    HWND hwnd = CreateWindowExW(0, CN, L"Batch Menu Generator",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        x, y, W, H, NULL, NULL, hI, NULL);

    if (!hwnd) {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nC);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}