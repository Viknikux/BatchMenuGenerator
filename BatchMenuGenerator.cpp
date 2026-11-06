//Created with God, love and electricity, BMG 1.5.0 Release-Stable
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#pragma comment(lib, "Shell32.lib")
#include <commdlg.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>
#include <cwctype>
#include <cstring>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <algorithm>
#include <functional>
#undef min
#undef max
#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "uuid.lib")

enum ConsoleColor {
    CC_BLACK        = 0,
    CC_RESET        = 7,
    CC_CYAN         = 11,
    CC_YELLOW       = 14,
    CC_GREEN        = 10,
    CC_RED          = 12,
    CC_WHITE        = 15,
    CC_MAGENTA      = 13,
    CC_DARK_GREEN   = 2,
    CC_DARK_RED     = 4,
    CC_DARK_YELLOW  = 6,
    CC_DARK_WHITE   = 8,
    CC_BRIGHT_GREEN = 10,
    CC_BRIGHT_CYAN  = 11,
    CC_BRIGHT_RED   = 12,
    CC_BRIGHT_WHITE = 15,
    CC_VIOLET        = 13,
    CC_DARK_VIOLET   = 5,
    CC_GRAY          = 8,
    CC_LIGHT_GRAY    = 7,
    CC_BLUE          = 9,
    CC_DARK_BLUE     = 1
};

struct Theme {
    int header;
    int prompt;
    int input;
    int success;
    int error;
    int info;
    int banner;
    int text;
    int accent;
};

static Theme g_theme = { CC_CYAN, CC_YELLOW, CC_WHITE, CC_GREEN, CC_RED, CC_MAGENTA, CC_CYAN, CC_WHITE, CC_YELLOW };

static void SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
}

static void ApplyTheme(const std::wstring& themeName) {
    if (themeName == L"matrix") {
        g_theme = { CC_DARK_GREEN, CC_GREEN, CC_BRIGHT_GREEN, CC_GREEN, CC_DARK_GREEN, CC_GREEN, CC_DARK_GREEN, CC_GREEN, CC_BRIGHT_GREEN };
    } else if (themeName == L"cyberpunk") {
        g_theme = { CC_CYAN, CC_YELLOW, CC_MAGENTA, CC_GREEN, CC_RED, CC_MAGENTA, CC_CYAN, CC_WHITE, CC_YELLOW };
    } else if (themeName == L"cli") {
        g_theme = { CC_LIGHT_GRAY, CC_GRAY, CC_BRIGHT_WHITE, CC_WHITE, CC_DARK_WHITE, CC_LIGHT_GRAY, CC_GRAY, CC_BRIGHT_WHITE, CC_WHITE };
    } else if (themeName == L"violent") {
        g_theme = { CC_RED, CC_BRIGHT_RED, CC_MAGENTA, CC_GREEN, CC_DARK_RED, CC_VIOLET, CC_DARK_RED, CC_WHITE, CC_BRIGHT_RED };
    } else {
        g_theme = { CC_CYAN, CC_YELLOW, CC_WHITE, CC_GREEN, CC_RED, CC_MAGENTA, CC_CYAN, CC_WHITE, CC_YELLOW };
    }
}

#define C_H SetColor(g_theme.header)
#define C_P SetColor(g_theme.prompt)
#define C_I SetColor(g_theme.input)
#define C_S SetColor(g_theme.success)
#define C_E SetColor(g_theme.error)
#define C_N SetColor(g_theme.info)
#define C_B SetColor(g_theme.banner)
#define C_T SetColor(g_theme.text)
#define C_A SetColor(g_theme.accent)
#define C_R SetColor(CC_RESET)

static std::wstring ReadLine() {
    std::wstring line;
    std::getline(std::wcin, line);
    while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n')) {
        line.pop_back();
    }
    return line;
}

// ====================================================================
// Dynamic title-centering engine.
// The banner geometry and the live console metrics are re-measured on
// every single call, so banner titles stay dead-center no matter the
// console size, the font, or whether wide/CJK glyphs are involved.
// ====================================================================

// Columns a string occupies on screen: East-Asian wide/fullwidth glyphs
// take two columns, all other characters take one.
static int TuiTextWidth(const std::wstring& text) {
    int width = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        const unsigned int c = static_cast<unsigned int>(text[i]);
        const bool wide =
            (c >= 0x1100 && c <= 0x115F) || // Hangul Jamo
            (c >= 0x2E80 && c <= 0xA4CF) || // CJK radicals .. Yi syllables
            (c >= 0xAC00 && c <= 0xD7A3) || // Hangul syllables
            (c >= 0xF900 && c <= 0xFAFF) || // CJK compatibility ideographs
            (c >= 0xFE30 && c <= 0xFE4F) || // CJK compatibility forms
            (c >= 0xFF00 && c <= 0xFF60) || // fullwidth forms
            (c >= 0xFFE0 && c <= 0xFFE6);   // fullwidth signs / arrows
        width += wide ? 2 : 1;
    }
    return width;
}

// Live console buffer width in columns (80 when it cannot be read).
static int TuiConsoleWidth() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hOut != INVALID_HANDLE_VALUE &&
        GetConsoleScreenBufferInfo(hOut, &csbi) &&
        csbi.dwSize.X > 0) {
        return csbi.dwSize.X;
    }
    return 80;
}

// "=" border line of the standard banner box (uses the active color).
//     ============================================
static void TuiBannerBorder(int boxInterior = 44, int leftMargin = 2) {
    const int margin = leftMargin > 0 ? leftMargin : 0;
    const int equals = boxInterior > 0 ? boxInterior : 0;
    wprintf(L"%ls%ls\n",
        std::wstring((size_t)margin, L' ').c_str(),
        std::wstring((size_t)equals, L'=').c_str());
}

// Print one line centered inside the banner box (uses the active color).
// Centering is recomputed from TuiTextWidth() on every call, so there is
// no hand-tuned space padding anywhere. To be safe, if the console is
// too narrow for the box, the line is centered across the console width.
static void TuiBannerTitle(const std::wstring& text, int boxInterior = 44, int leftMargin = 2) {
    const int consoleWidth = TuiConsoleWidth();
    const int margin = leftMargin > 0 ? leftMargin : 0;
    const int interior = boxInterior > 0 ? boxInterior : 0;
    const int textWidth = TuiTextWidth(text);
    int leading;
    if (margin + interior > consoleWidth) {
        leading = (consoleWidth - textWidth) / 2; // narrow console: center on screen instead
    } else {
        leading = margin + (interior - textWidth) / 2;
    }
    if (leading < 0) leading = 0;
    wprintf(L"%ls%ls\n",
        std::wstring((size_t)leading, L' ').c_str(),
        text.c_str());
}

static std::wstring Trim(const std::wstring& str) {
    if (str.empty()) return L"";
    size_t first = 0;
    while (first < str.length() && str[first] <= 32) {
        first++;
    }
    if (first == str.length()) return L"";
    size_t last = str.length() - 1;
    while (last > first && str[last] <= 32) {
        last--;
    }
    return str.substr(first, (last - first + 1));
}

static std::wstring ConvertToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static std::wstring EscapeBatch(const std::wstring& s, bool c) {
    std::wstring r; r.reserve(s.length() * 2);
    for (auto ch : s) {
        switch (ch) {
        case L'%': r += L"%%"; break;
        case L'^': r += L"^^"; break;
        case L'&': r += L"^&"; break;
        case L'<': r += L"^<"; break;
        case L'>': r += L"^>"; break;
        case L'|': r += L"^|"; break;
        case L'"': r += c ? L"\"\"" : L"^\""; break;
        default:   r += ch; break;
        }
    }
    return r;
}

static std::wstring MakeBorder(wchar_t ch);

static std::wstring UnescapeBatch(const std::wstring& s, bool cmdC = false) {
    std::wstring r;
    r.reserve(s.length());
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] == L'^' && i + 1 < s.length()) {
            wchar_t next = s[i + 1];
            if (next == L'^' || next == L'&' || next == L'<' || next == L'>' || next == L'|' || next == L'"') {
                r += next;
                i++;
                continue;
            }
        }
        if (s[i] == L'%' && i + 1 < s.length() && s[i + 1] == L'%') {
            r += L'%';
            i++;
            continue;
        }
        if (cmdC && s[i] == L'"' && i + 1 < s.length() && s[i + 1] == L'"') {
            r += L'"';
            i++;
            continue;
        }
        r += s[i];
    }
    return r;
}

struct OptionData {
    std::wstring label;
    std::vector<std::wstring> commands;
    int runMode = 0;
    std::vector<OptionData> subOptions;
};

static bool ConsoleTextEditor(std::vector<std::wstring>& lines, const std::wstring& savePath = L"");
static void EditLinesWithConsoleEditor(std::vector<std::wstring>& lines, bool removeEmptyLines = false);
static bool ReadConsoleEditorTextFile(const std::wstring& path, std::vector<std::wstring>& lines);

static bool g_enableTUI = true;
static bool g_enableSound = false;

static void SetInvertedColor(int bgColor, int fgColor = CC_BLACK) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)(((bgColor & 0x0F) << 4) | (fgColor & 0x0F)));
}

static void PlayMenuSound(int type) {
    if (!g_enableSound) return;
    if (type == 0) Beep(750, 20);
    else if (type == 1) Beep(1150, 45);
}

static void EnsureConsoleBufferRows(HANDLE output, SHORT requiredBottom);

static int RunTUIMenu(const std::wstring& menuTitle, const std::vector<std::wstring>& options,
    int defaultIdx = 0, int confirmedIdx = -1, bool clearScreen = true) {
    int currentIdx = defaultIdx;
    int count = (int)options.size();
    if (count == 0) return -1;
    if (currentIdx < 0) currentIdx = 0;
    if (currentIdx >= count) currentIdx = count - 1;

    CONSOLE_CURSOR_INFO cursorInfo{};
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);

    if (clearScreen) system("cls");
    else wprintf(L"\n");

    C_B; TuiBannerBorder();
    C_H; TuiBannerTitle(menuTitle);
    C_B; TuiBannerBorder();
    wprintf(L"\n"); C_R;
    fflush(stdout);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    int startY = csbi.dwCursorPosition.Y;
    EnsureConsoleBufferRows(hOut, (SHORT)(startY + count + 1));

    auto PrintOption = [&](int i, bool isSelected) {
        COORD pos = { 0, (SHORT)(startY + i) };
        SetConsoleCursorPosition(hOut, pos);
        CONSOLE_SCREEN_BUFFER_INFO rowInfo{};
        const DWORD width = GetConsoleScreenBufferInfo(hOut, &rowInfo) ? (DWORD)rowInfo.dwSize.X : 1;
        const int rowWidth = width > 1 ? (int)width - 1 : 1;
        for (int column = 0; column < rowWidth; ++column) putwchar(L' ');
        putwchar(L'\r');
        std::wstring display;
        if (i == confirmedIdx) {
            C_S;
            display = L"    [X] " + options[i] + L"  (Selected)";
        } else if (isSelected) {
            C_R;
            wprintf(L"    ");
            SetInvertedColor(g_theme.accent, CC_BLACK);
            display = L" > " + options[i] + L" ";
        } else {
            C_T;
            display = L"        " + options[i];
        }
        const int prefixWidth = isSelected && i != confirmedIdx ? 4 : 0;
        const size_t maxText = (size_t)(rowWidth > prefixWidth ? rowWidth - prefixWidth : 0);
        if (display.size() > maxText) display.resize(maxText);
        if (!display.empty()) wprintf(L"%ls", display.c_str());
        C_R;
        fflush(stdout);
    };

    for (int i = 0; i < count; i++) {
        PrintOption(i, i == currentIdx);
    }

    const SHORT menuEndY = (SHORT)(startY + count + 1);
    COORD menuEndPos = { 0, menuEndY };
    SetConsoleCursorPosition(hOut, menuEndPos);
    fflush(stdout);

    auto finishMenu = [&](int result) {
        cursorInfo.bVisible = TRUE;
        SetConsoleCursorInfo(hOut, &cursorInfo);
        SetConsoleCursorPosition(hOut, menuEndPos);
        return result;
    };
    auto moveSelection = [&](int direction) {
        const int oldIdx = currentIdx;
        currentIdx = (currentIdx + direction + count) % count;
        if (oldIdx != currentIdx) {
            PrintOption(oldIdx, false);
            PrintOption(currentIdx, true);
            PlayMenuSound(0);
        }
    };

    while (true) {
        INPUT_RECORD input{};
        DWORD eventsRead = 0;
        if (!ReadConsoleInputW(hIn, &input, 1, &eventsRead) || eventsRead == 0) continue;

        if (input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
            const KEY_EVENT_RECORD& keyEvent = input.Event.KeyEvent;
            if (keyEvent.wVirtualKeyCode == VK_UP) {
                moveSelection(-1);
            } else if (keyEvent.wVirtualKeyCode == VK_DOWN) {
                moveSelection(1);
            } else if (keyEvent.wVirtualKeyCode == VK_RETURN) {
                PlayMenuSound(1);
                return finishMenu(currentIdx);
            } else if (keyEvent.wVirtualKeyCode == VK_ESCAPE) {
                return finishMenu(-1);
            }
        }
    }
}

static std::wstring ReadOptionCountInput() {
    if (!g_enableTUI) return ReadLine();

    fflush(stdout);

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo{};
    GetConsoleCursorInfo(hOut, &cursorInfo);
    const BOOL oldCursorVisible = cursorInfo.bVisible;
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hOut, &cursorInfo);

    CONSOLE_SCREEN_BUFFER_INFO screenInfo{};
    GetConsoleScreenBufferInfo(hOut, &screenInfo);
    const COORD inputPos = screenInfo.dwCursorPosition;
    const int availableWidth = (std::max)(1, (int)screenInfo.dwSize.X - (int)inputPos.X);
    std::wstring typed;

    auto visibleValue = [&]() -> std::wstring {
        return typed;
    };
    auto redrawInput = [&]() {
        const std::wstring value = visibleValue();
        DWORD written = 0;
        FillConsoleOutputCharacterW(hOut, L' ', (DWORD)availableWidth, inputPos, &written);
        SetConsoleCursorPosition(hOut, inputPos);
        if (!value.empty()) wprintf(L"%ls", value.c_str());
        fflush(stdout);
    };
    auto finishInput = [&](const std::wstring& result) {
        wprintf(L"\n");
        fflush(stdout);
        cursorInfo.bVisible = oldCursorVisible;
        SetConsoleCursorInfo(hOut, &cursorInfo);
        return result;
    };

    while (true) {
        INPUT_RECORD input{};
        DWORD eventsRead = 0;
        if (!ReadConsoleInputW(hIn, &input, 1, &eventsRead) || eventsRead == 0) continue;

        if (input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
            const KEY_EVENT_RECORD& keyEvent = input.Event.KeyEvent;
            if (keyEvent.wVirtualKeyCode == VK_RETURN) {
                return finishInput(visibleValue());
            }
            if (keyEvent.wVirtualKeyCode == VK_ESCAPE) {
                typed.clear();
                redrawInput();
                return finishInput(L"");
            }
            if (keyEvent.wVirtualKeyCode == VK_BACK) {
                if (!typed.empty()) typed.pop_back();
                redrawInput();
                continue;
            }
            const wchar_t ch = keyEvent.uChar.UnicodeChar;
            if (ch >= L'0' && ch <= L'9' && (int)typed.size() < availableWidth) {
                typed.push_back(ch);
                redrawInput();
            }
        }
    }
}

static std::wstring ReadTitleWithPreview(std::wstring currentTitle = L"") {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo{};
    GetConsoleCursorInfo(hOut, &cursorInfo);
    BOOL oldVisible = cursorInfo.bVisible;
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hOut, &cursorInfo);

    const int WIDTH = 57;
    std::wstring title = currentTitle;
    if (title.length() > WIDTH) title.resize(WIDTH);

    C_P; wprintf(L"  Enter title: "); C_I;
    if (!title.empty()) wprintf(L"%ls", title.c_str());

    COORD inputPos{};
    COORD previewPos{};
    COORD layoutEndPos{};
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    GetConsoleScreenBufferInfo(hOut, &csbi);
    inputPos = csbi.dwCursorPosition;

    C_R; wprintf(L"\n\n");
    C_B; wprintf(L"%ls\n", MakeBorder(L'=').c_str());
    GetConsoleScreenBufferInfo(hOut, &csbi);
    previewPos = csbi.dwCursorPosition;
    C_H; wprintf(L"%*ls", WIDTH, L""); C_R;
    wprintf(L"\n");
    C_B; wprintf(L"%ls\n", MakeBorder(L'=').c_str());
    wprintf(L"\n\n");
    GetConsoleScreenBufferInfo(hOut, &csbi);
    layoutEndPos = csbi.dwCursorPosition;

    SetConsoleCursorPosition(hOut, inputPos);

    auto redrawPreview = [&]() {
        std::wstring line(WIDTH, L' ');
        int len = (int)title.length();
        int left = (WIDTH - len) / 2;
        if (left < 0) left = 0;
        if (len > WIDTH) len = WIDTH;
        for (int i = 0; i < len; ++i) line[left + i] = title[(size_t)i];

        COORD saved{};
        CONSOLE_SCREEN_BUFFER_INFO s{};
        GetConsoleScreenBufferInfo(hOut, &s);
        saved = s.dwCursorPosition;
        SetConsoleCursorPosition(hOut, previewPos);
        C_H;
        wprintf(L"%ls", line.c_str());
        C_R;
        SetConsoleCursorPosition(hOut, saved);
    };

    redrawPreview();
    SetConsoleCursorPosition(hOut, inputPos);

    while (true) {
        int c = _getwch();

        if (c == 13) {
            cursorInfo.bVisible = oldVisible;
            SetConsoleCursorInfo(hOut, &cursorInfo);
            SetConsoleCursorPosition(hOut, layoutEndPos);
            return title;
        }

        if (c == 27) {
            cursorInfo.bVisible = oldVisible;
            SetConsoleCursorInfo(hOut, &cursorInfo);
            SetConsoleCursorPosition(hOut, layoutEndPos);
            return L"__BMG_CANCEL__";
        }

        if (c == 8) {
            if (!title.empty()) {
                title.pop_back();
                wprintf(L"\b \b");
                redrawPreview();
            }
            continue;
        }

        if (c == 0 || c == 224) {
            (void)_getwch();
            continue;
        }

        if (c >= 32 && c != 127) {
            if (title.length() < WIDTH) {
                wchar_t wc = (wchar_t)c;
                title.push_back(wc);
                wprintf(L"%lc", wc);
                redrawPreview();
            } else {
                Beep(750, 25);
            }
        }
    }
}


static std::wstring FormatOptionSummary(const OptionData& opt, size_t index) {
    std::wstring result = L"Option " + std::to_wstring(index + 1) + L": " +
        (opt.label.empty() ? (L"Option " + std::to_wstring(index + 1)) : opt.label);

    if (opt.runMode == 2) {
        result += L" [Submenu";
        if (!opt.subOptions.empty()) {
            result += L": ";
            for (size_t i = 0; i < opt.subOptions.size(); ++i) {
                if (i > 0) result += L", ";
                result += std::to_wstring(i + 1) + L") " +
                    (opt.subOptions[i].label.empty()
                        ? (L"Option " + std::to_wstring(i + 1))
                        : opt.subOptions[i].label);
                if (opt.subOptions[i].runMode == 2) result += L" [Submenu]";
            }
        }
        result += L"]";
    } else {
        result += L" (" + std::to_wstring(opt.commands.size()) +
            L" cmd(s), RunMode " + std::to_wstring(opt.runMode) + L")";
    }

    const size_t MAX_DISPLAY = 150;
    if (result.size() > MAX_DISPLAY) result = result.substr(0, MAX_DISPLAY - 3) + L"...";
    return result;
}

static void EditDescriptionTUI(std::vector<std::wstring>& description) {
    EditLinesWithConsoleEditor(description);
}

static void EditCommandsTUI(std::vector<std::wstring>& commands) {
    EditLinesWithConsoleEditor(commands, true);
}
static int PromptRunMode(int currentMode, bool allowSubmenu = true) {
    if (currentMode < 0 || currentMode > 2) currentMode = 0;

    if (g_enableTUI) {
        std::vector<std::wstring> choices = {
            L"Direct call / CALL",
            L"cmd /c wrapper (RunMode 1)",
        };
        if (allowSubmenu) choices.push_back(L"Sub menu");
        int sel = RunTUIMenu(L"Select Run Mode", choices, currentMode, -1, false);
        if (sel < 0) return currentMode;
        return sel;
    }

    while (true) {
        C_P; wprintf(L"  Run mode:\n");
        wprintf(L"    0 = Direct call (call command)\n");
        wprintf(L"    1 = cmd /c wrapper\n");
        if (allowSubmenu) wprintf(L"    2 = Sub menu\n");
        wprintf(L"  Select [%d]: ", currentMode); C_I;
        std::wstring s = ReadLine(); C_R;
        if (s.empty()) return currentMode;
        int v = _wtoi(s.c_str());
        if (v == 0 || v == 1 || (allowSubmenu && v == 2)) return v;
        C_E; wprintf(L"  [Error] Invalid run mode.\n"); C_R;
    }
}

static void ConfigureSubmenuInteractive(std::vector<OptionData>& subOptions, int depth = 0) {
    const int MAX_DEPTH = 6;
    if (depth >= MAX_DEPTH) {
        C_E; wprintf(L"  [Error] Maximum submenu depth reached.\n"); C_R;
        return;
    }

    int optionCount = subOptions.empty() ? 2 : (int)subOptions.size();
    while (true) {
        C_P; wprintf(L"  Number of submenu options (1-20) [%d]: ", optionCount); C_I;
        std::wstring s = ReadOptionCountInput(); C_R;
        if (s.empty()) break;
        int v = _wtoi(s.c_str());
        if (v >= 1 && v <= 20) { optionCount = v; break; }
        C_E; wprintf(L"  [Error] Enter a value between 1 and 20.\n"); C_R;
    }

    subOptions.resize((size_t)optionCount);

    for (size_t i = 0; i < subOptions.size(); ++i) {
        C_N; wprintf(L"\n  --- Submenu Option %zu ---\n", i + 1); C_R;

        C_P;
        if (!subOptions[i].label.empty())
            wprintf(L"  Label [%ls]: ", subOptions[i].label.c_str());
        else
            wprintf(L"  Label [Option %zu]: ", i + 1);
        C_I;
        std::wstring label = ReadLine(); C_R;
        if (!label.empty()) subOptions[i].label = label;
        if (subOptions[i].label.empty()) subOptions[i].label = L"Option " + std::to_wstring(i + 1);

        int newMode = PromptRunMode(subOptions[i].runMode, true);
        subOptions[i].runMode = newMode;

        if (newMode == 2) {
            subOptions[i].commands.clear();
            ConfigureSubmenuInteractive(subOptions[i].subOptions, depth + 1);
            continue;
        }

        subOptions[i].subOptions.clear();
        if (!subOptions[i].commands.empty()) {
            C_S; wprintf(L"  Existing command(s):\n");
            for (const auto& cmd : subOptions[i].commands) wprintf(L"    > %ls\n", cmd.c_str());
            C_R;
        }

        C_P; wprintf(L"  Enter command(s), one per line. Empty line when done.\n"); C_R;
        while (true) {
            C_I; wprintf(L"    > "); std::wstring cmd = ReadLine(); C_R;
            if (cmd.empty()) break;
            subOptions[i].commands.push_back(cmd);
        }
    }
}

struct AppConfig {
    bool theme_engine = false;
    std::wstring theme_preset = L"default";
    bool autosave = true;
    int end = 0;
    int saveload = 0;
    int menuborder = 0;
    int bconfig = 1;
    int tui = 1;
    int sound = 0;
};

static AppConfig g_appConfig;

static void RunVersionHistoryMenu();
static std::wstring GetExecutableFilePath();

static std::wstring GetExePath() {
    std::wstring path = GetExecutableFilePath();
    if (path.empty()) return L".\\";
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) return path.substr(0, pos + 1);
    return L".\\";
}

static std::wstring GetBatchMenuGeneratorAppDataDir(bool createDirectory = true) {
    wchar_t appData[MAX_PATH] = {0};
    DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", appData, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        std::wstring dir = std::wstring(appData) + L"\\BatchMenuGenerator\\";
        if (createDirectory) CreateDirectoryW(dir.c_str(), NULL);
        return dir;
    }

    wchar_t tempDir[MAX_PATH] = {0};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring dir = std::wstring(tempDir) + L"BatchMenuGenerator\\";
    if (createDirectory) CreateDirectoryW(dir.c_str(), NULL);
    return dir;
}

static bool PathExists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

static bool DirectoryExists(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static std::wstring GetSettingsFilePath() {
    const std::wstring appDataDir = GetBatchMenuGeneratorAppDataDir();
    const std::wstring target = appDataDir + L"settings.txt";

    if (!PathExists(target)) {
        const std::wstring legacySettings = GetExePath() + L"settings.txt";
        const std::wstring legacyBconfig = GetExePath() + L"bconfig.txt";
        std::wstring legacySource;

        if (PathExists(legacySettings)) legacySource = legacySettings;
        else if (PathExists(legacyBconfig)) legacySource = legacyBconfig;

        if (!legacySource.empty()) {
            if (!MoveFileExW(legacySource.c_str(), target.c_str(),
                MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
            }
        }
    }

    return target;
}

static bool SaveSettingsFile(const AppConfig& cfg) {
    std::wstring path = GetSettingsFilePath();
    HANDLE hf = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return false;

    std::string content = "theme: " + std::string(cfg.theme_engine ? "true" : "false") + "\r\n";

    int len = WideCharToMultiByte(CP_UTF8, 0, cfg.theme_preset.c_str(), -1, NULL, 0, NULL, NULL);
    if (len <= 0) {
        CloseHandle(hf);
        return false;
    }

    std::vector<char> buf(len);
    if (WideCharToMultiByte(CP_UTF8, 0, cfg.theme_preset.c_str(), -1, buf.data(), len, NULL, NULL) <= 0) {
        CloseHandle(hf);
        return false;
    }
    content += "themepreset: " + std::string(buf.data()) + "\r\n";

    content += "autosave: " + std::string(cfg.autosave ? "true" : "false") + "\r\n";
    content += "end: " + std::to_string(cfg.end) + "\r\n";
    content += "saveload: " + std::to_string(cfg.saveload) + "\r\n";
    content += "menuborder: " + std::to_string(cfg.menuborder) + "\r\n";
    content += "bconfig: " + std::to_string(cfg.bconfig) + "\r\n";
    content += "tui: " + std::to_string(cfg.tui) + "\r\n";
    content += "sound: " + std::to_string(cfg.sound) + "\r\n";

    DWORD wr = 0;
    BOOL ok = WriteFile(hf, content.c_str(), (DWORD)content.length(), &wr, NULL);
    CloseHandle(hf);
    return ok && wr == content.length();
}

static void LoadSettingsFile(AppConfig& cfg, std::vector<std::wstring>& warnings) {
    std::wstring path = GetSettingsFilePath();
    DWORD attr = GetFileAttributesW(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        SaveSettingsFile(cfg);
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        std::wstring wline = Trim(ConvertToWide(line));
        if (wline.empty() || wline.substr(0, 2) == L"//") continue;

        size_t colon = wline.find(L':');
        if (colon == std::wstring::npos) continue;

        std::wstring key = Trim(wline.substr(0, colon));
        std::wstring val = Trim(wline.substr(colon + 1));

        std::transform(key.begin(), key.end(), key.begin(), ::towlower);

        if (key == L"theme") {
            if (val == L"true" || val == L"1") cfg.theme_engine = true;
            else cfg.theme_engine = false;
        } else if (key == L"themepreset") {
            std::wstring lval = val;
            std::transform(lval.begin(), lval.end(), lval.begin(), ::towlower);
            if (lval == L"default" || lval == L"matrix" || lval == L"cyberpunk" || lval == L"cli" || lval == L"violent") {
                cfg.theme_preset = lval;
            } else {
                warnings.push_back(L"\"" + val + L"\" is not a recognized theme preset. Valid options are: default, matrix, cyberpunk, cli, violent");
            }
        } else if (key == L"autosave") {
            cfg.autosave = (val == L"true" || val == L"1");
        } else if (key == L"end") {
            int v = _wtoi(val.c_str());
            if (v >= 0 && v <= 3) cfg.end = v;
            else warnings.push_back(L"\"" + val + L"\" is not a recognized end option. Valid options are: 0 (Prompt), 1 (Main Menu), 2 (Launch), 3 (Exit)");
        } else if (key == L"saveload" || key == L"save/load" || key == L"load") {
            int v = _wtoi(val.c_str());
            if (v >= 0 && v <= 2) cfg.saveload = v;
            else warnings.push_back(L"\"" + val + L"\" is not a recognized save/load option. Valid options are: 0 (Prompt), 1 (Explorer), 2 (Manual)");
        } else if (key == L"menuborder") {
            int v = _wtoi(val.c_str());
            if (v >= 0 && v <= 3) cfg.menuborder = v;
            else warnings.push_back(L"\"" + val + L"\" is not a recognized menuborder option. Valid options are: 0 (Prompt), 1 (Classic), 2 (Modern), 3 (ZigZag)");
        } else if (key == L"bconfig") {
            int v = _wtoi(val.c_str());
            if (v == 0 || v == 1) cfg.bconfig = v;
            else warnings.push_back(L"\"" + val + L"\" is not a recognized bconfig option. Valid options are: 0 (Disabled), 1 (Enabled)");
        } else if (key == L"tui") {
            int v = _wtoi(val.c_str());
            if (v == 0 || v == 1) { cfg.tui = v; g_enableTUI = (v == 1); }
        } else if (key == L"sound") {
            int v = _wtoi(val.c_str());
            if (v == 0 || v == 1) { cfg.sound = v; g_enableSound = (v == 1); }
        }
    }
    file.close();

    if (cfg.theme_engine) ApplyTheme(cfg.theme_preset);
    else ApplyTheme(L"default");
}

static void ReloadSettingsFromDisk(std::vector<std::wstring>& warnings) {
    AppConfig fresh;
    std::vector<std::wstring> localWarnings;
    LoadSettingsFile(fresh, localWarnings);

    g_appConfig = fresh;
    g_enableTUI = (fresh.tui == 1);
    g_enableSound = (fresh.sound == 1);

    warnings.insert(warnings.end(), localWarnings.begin(), localWarnings.end());

    if (g_appConfig.theme_engine) ApplyTheme(g_appConfig.theme_preset);
    else ApplyTheme(L"default");
}

static bool g_exitAfterMove = false;

static std::wstring GetExecutableFilePath() {
    std::vector<wchar_t> modulePath(32768, L'\0');
    DWORD moduleLength = GetModuleFileNameW(NULL, modulePath.data(), (DWORD)modulePath.size());
    if (moduleLength == 0 || moduleLength >= modulePath.size()) return L"";
    modulePath.resize(moduleLength);
    std::wstring path(modulePath.begin(), modulePath.end());

    HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (file == INVALID_HANDLE_VALUE) return path;

    std::vector<wchar_t> finalPath(32768, L'\0');
    DWORD finalLength = GetFinalPathNameByHandleW(file, finalPath.data(),
        (DWORD)finalPath.size(), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    CloseHandle(file);
    if (finalLength == 0 || finalLength >= finalPath.size()) return path;
    std::wstring resolved(finalPath.data(), finalLength);
    if (resolved.size() < MAX_PATH && resolved.rfind(L"\\\\?\\UNC\\", 0) == 0)
        return L"\\\\" + resolved.substr(8);
    if (resolved.size() < MAX_PATH && resolved.rfind(L"\\\\?\\", 0) == 0)
        return resolved.substr(4);
    return resolved;
}

static std::wstring GetExecutableDirectory(const std::wstring& exePath) {
    size_t separator = exePath.find_last_of(L"\\/");
    return separator == std::wstring::npos ? L"" : exePath.substr(0, separator + 1);
}

static std::wstring NormalizePathForCompare(std::wstring path) {
    std::replace(path.begin(), path.end(), L'/', L'\\');
    while (path.size() > 3 && path.back() == L'\\') path.pop_back();
    std::transform(path.begin(), path.end(), path.begin(), ::towlower);
    return path;
}

static std::wstring QuoteCommandLineArgument(const std::wstring& value) {
    std::wstring quoted = L"\"";
    size_t slashes = 0;
    for (wchar_t ch : value) {
        if (ch == L'\\') {
            ++slashes;
        } else if (ch == L'\"') {
            quoted.append(slashes * 2 + 1, L'\\');
            quoted += L'\"';
            slashes = 0;
        } else {
            quoted.append(slashes, L'\\');
            slashes = 0;
            quoted += ch;
        }
    }
    quoted.append(slashes * 2, L'\\');
    quoted += L'\"';
    return quoted;
}

static bool GetKnownFolderPath(int folder, std::wstring& path) {
    wchar_t buffer[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(NULL, folder | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, buffer)))
        return false;
    path = buffer;
    return !path.empty();
}

static bool CreateOrUpdateShortcut(const std::wstring& shortcutPath, const std::wstring& targetPath) {
    HRESULT initResult = CoInitialize(NULL);
    if (FAILED(initResult) && initResult != RPC_E_CHANGED_MODE) return false;
    const bool uninitialize = SUCCEEDED(initResult);

    IShellLinkW* link = NULL;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
        IID_IShellLinkW, (void**)&link);
    if (SUCCEEDED(result)) {
        link->SetPath(targetPath.c_str());
        link->SetWorkingDirectory(GetExecutableDirectory(targetPath).c_str());
        link->SetIconLocation(targetPath.c_str(), 0);

        IPersistFile* persist = NULL;
        result = link->QueryInterface(IID_IPersistFile, (void**)&persist);
        if (SUCCEEDED(result)) {
            result = persist->Save(shortcutPath.c_str(), TRUE);
            persist->Release();
        }
        link->Release();
    }
    if (uninitialize) CoUninitialize();
    return SUCCEEDED(result);
}

static std::wstring GetShortcutPath(bool desktop) {
    std::wstring folder;
    if (!GetKnownFolderPath(desktop ? CSIDL_DESKTOPDIRECTORY : CSIDL_PROGRAMS, folder)) return L"";
    return folder + L"\\BatchMenuGenerator.lnk";
}

static bool CreateProgramShortcut(bool desktop) {
    const std::wstring target = GetExecutableFilePath();
    const std::wstring shortcut = GetShortcutPath(desktop);
    return !target.empty() && !shortcut.empty() && CreateOrUpdateShortcut(shortcut, target);
}

static bool UpdateShortcutIfPresent(bool desktop, const std::wstring& targetPath) {
    const std::wstring shortcut = GetShortcutPath(desktop);
    if (shortcut.empty() || GetFileAttributesW(shortcut.c_str()) == INVALID_FILE_ATTRIBUTES) return true;
    return CreateOrUpdateShortcut(shortcut, targetPath);
}

static std::wstring GetBmgCommandDirectory() {
    return GetBatchMenuGeneratorAppDataDir() + L"commands\\";
}

static std::wstring ExpandPathEntry(const std::wstring& entry) {
    std::wstring source = Trim(entry);
    if (source.size() >= 2 && source.front() == L'\"' && source.back() == L'\"')
        source = source.substr(1, source.size() - 2);
    DWORD needed = ExpandEnvironmentStringsW(source.c_str(), NULL, 0);
    if (needed == 0) return source;
    std::vector<wchar_t> expanded(needed, L'\0');
    DWORD written = ExpandEnvironmentStringsW(source.c_str(), expanded.data(), needed);
    return written > 0 && written <= needed ? std::wstring(expanded.data()) : source;
}

static bool ReadUserPathValue(std::wstring& path, DWORD& valueType) {
    HKEY key = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD bytes = 0;
    LONG result = RegQueryValueExW(key, L"Path", NULL, &valueType, NULL, &bytes);
    if (result == ERROR_FILE_NOT_FOUND) {
        path.clear();
        valueType = REG_EXPAND_SZ;
        RegCloseKey(key);
        return true;
    }
    if (result != ERROR_SUCCESS) {
        RegCloseKey(key);
        return false;
    }
    std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 1, L'\0');
    result = RegQueryValueExW(key, L"Path", NULL, &valueType,
        (LPBYTE)buffer.data(), &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) return false;
    path.assign(buffer.data());
    if (valueType != REG_SZ && valueType != REG_EXPAND_SZ) valueType = REG_EXPAND_SZ;
    return true;
}

static std::vector<std::wstring> SplitPathEntries(const std::wstring& path) {
    std::vector<std::wstring> entries;
    size_t start = 0;
    while (start <= path.size()) {
        size_t separator = path.find(L';', start);
        if (separator == std::wstring::npos) separator = path.size();
        std::wstring entry = Trim(path.substr(start, separator - start));
        if (!entry.empty()) entries.push_back(entry);
        if (separator == path.size()) break;
        start = separator + 1;
    }
    return entries;
}

static bool PathContainsCommandDirectory(const std::wstring& pathValue) {
    const std::wstring wanted = NormalizePathForCompare(GetBmgCommandDirectory());
    const std::vector<std::wstring> entries = SplitPathEntries(pathValue);
    for (const auto& entry : entries) {
        if (NormalizePathForCompare(ExpandPathEntry(entry)) == wanted) return true;
    }
    return false;
}

static bool ModifyUserPath(bool addDirectory) {
    HKEY key = NULL;
    DWORD disposition = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Environment", 0, NULL, 0,
        KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &key, &disposition) != ERROR_SUCCESS)
        return false;

    std::wstring pathValue;
    DWORD valueType = REG_EXPAND_SZ;
    DWORD bytes = 0;
    LONG queryResult = RegQueryValueExW(key, L"Path", NULL, &valueType, NULL, &bytes);
    if (queryResult == ERROR_SUCCESS) {
        std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 1, L'\0');
        queryResult = RegQueryValueExW(key, L"Path", NULL, &valueType,
            (LPBYTE)buffer.data(), &bytes);
        if (queryResult == ERROR_SUCCESS) pathValue.assign(buffer.data());
    } else if (queryResult == ERROR_FILE_NOT_FOUND) {
        valueType = REG_EXPAND_SZ;
    }
    if (queryResult != ERROR_SUCCESS && queryResult != ERROR_FILE_NOT_FOUND) {
        RegCloseKey(key);
        return false;
    }
    if (valueType != REG_SZ && valueType != REG_EXPAND_SZ) valueType = REG_EXPAND_SZ;

    std::vector<std::wstring> entries = SplitPathEntries(pathValue);
    const std::wstring wanted = NormalizePathForCompare(GetBmgCommandDirectory());
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const std::wstring& entry) {
        return NormalizePathForCompare(ExpandPathEntry(entry)) == wanted;
    }), entries.end());
    if (addDirectory) entries.push_back(GetBmgCommandDirectory());

    std::wstring updated;
    for (const auto& entry : entries) {
        if (!updated.empty()) updated += L';';
        updated += entry;
    }
    const DWORD updatedBytes = (DWORD)((updated.size() + 1) * sizeof(wchar_t));
    LONG setResult = RegSetValueExW(key, L"Path", 0, valueType,
        (const BYTE*)updated.c_str(), updatedBytes);
    RegCloseKey(key);
    if (setResult != ERROR_SUCCESS) return false;

    DWORD_PTR ignored = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment",
        SMTO_ABORTIFHUNG, 5000, &ignored);
    return true;
}

static bool FindFileEntry(const std::wstring& path, DWORD& attributes) {
    WIN32_FIND_DATAW data{};
    HANDLE find = FindFirstFileW(path.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) return false;
    FindClose(find);
    attributes = data.dwFileAttributes;
    return true;
}

static bool IsManagedBmgCommandShim(const std::wstring& path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    unsigned char bytes[256] = {0};
    DWORD read = 0;
    BOOL ok = ReadFile(file, bytes, sizeof(bytes), &read, NULL);
    CloseHandle(file);
    if (!ok || read < 4 || bytes[0] != 0xFF || bytes[1] != 0xFE) return false;
    std::wstring head((wchar_t*)(bytes + 2), (read - 2) / sizeof(wchar_t));
    return head.find(L"BatchMenuGenerator managed command") != std::wstring::npos;
}

static bool WriteBmgCommandShim(const std::wstring& path, const std::wstring& targetPath) {
    DWORD attributes = 0;
    if (FindFileEntry(path, attributes) && !IsManagedBmgCommandShim(path)) return false;

    std::wstring escapedTarget;
    for (wchar_t ch : targetPath) {
        escapedTarget += ch;
        if (ch == L'%') escapedTarget += L'%';
    }
    std::wstring content = L"\xFEFF@echo off\r\n@rem BatchMenuGenerator managed command\r\n\"" +
        escapedTarget + L"\" %*\r\nexit /b %ERRORLEVEL%\r\n";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const DWORD bytes = (DWORD)(content.size() * sizeof(wchar_t));
    BOOL ok = WriteFile(file, content.data(), bytes, &written, NULL);
    CloseHandle(file);
    return ok && written == bytes;
}

static bool InstallOrUpdateBmgCommands(const std::wstring& targetPath, std::wstring& message) {
    const std::wstring directory = GetBmgCommandDirectory();
    if (!CreateDirectoryW(directory.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        message = L"Could not create the BMG commands folder.";
        return false;
    }

    const std::wstring linkPath = directory + L"bmg.exe";
    const std::wstring shimPath = directory + L"bmg.cmd";
    DWORD attributes = 0;
    if (FindFileEntry(linkPath, attributes)) {
        if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0 || !DeleteFileW(linkPath.c_str())) {
            message = L"A non-symlink bmg.exe already exists in the commands folder; it was left untouched.";
            return false;
        }
    }
    if (FindFileEntry(shimPath, attributes) && IsManagedBmgCommandShim(shimPath)) {
        if (!DeleteFileW(shimPath.c_str())) {
            message = L"Could not replace the existing BMG command shim.";
            return false;
        }
    }

    bool usedShim = false;
    DWORD flags = 0x2;
    if (!CreateSymbolicLinkW(linkPath.c_str(), targetPath.c_str(), flags)) {
        DWORD error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER)
            CreateSymbolicLinkW(linkPath.c_str(), targetPath.c_str(), 0);
    }
    if (GetFileAttributesW(linkPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (!WriteBmgCommandShim(shimPath, targetPath)) {
            message = L"Windows could not create a symbolic link or the command shim. Enable Developer Mode or run BMG elevated, then try again.";
            return false;
        }
        usedShim = true;
    }

    if (!ModifyUserPath(true)) {
        message = L"The BMG command entry was created, but the user PATH could not be updated.";
        return false;
    }
    message = usedShim
        ? L"BMG commands are installed in the user PATH using a command shim. Open a new terminal to use bmg."
        : L"BMG symlink is installed in the user PATH. Open a new terminal to use bmg.";
    return true;
}

static bool UninstallBmgCommands(std::wstring& message) {
    const std::wstring directory = GetBmgCommandDirectory();
    bool ok = true;
    DWORD attributes = 0;
    const std::wstring linkPath = directory + L"bmg.exe";
    if (FindFileEntry(linkPath, attributes)) {
        if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) ok = DeleteFileW(linkPath.c_str()) != 0 && ok;
        else {
            message = L"A non-symlink bmg.exe was found in the commands folder and was left untouched.";
            ok = false;
        }
    }
    const std::wstring shimPath = directory + L"bmg.cmd";
    if (FindFileEntry(shimPath, attributes) && IsManagedBmgCommandShim(shimPath))
        ok = DeleteFileW(shimPath.c_str()) != 0 && ok;
    if (!ModifyUserPath(false)) {
        message = L"The command files were removed, but the user PATH could not be updated.";
        return false;
    }
    if (ok) message = L"BMG command links and the user PATH entry were removed.";
    return ok;
}

static bool BmgCommandsAreInstalled() {
    std::wstring pathValue;
    DWORD type = REG_EXPAND_SZ;
    if (ReadUserPathValue(pathValue, type) && PathContainsCommandDirectory(pathValue)) return true;
    DWORD attributes = 0;
    return FindFileEntry(GetBmgCommandDirectory() + L"bmg.exe", attributes) ||
        FindFileEntry(GetBmgCommandDirectory() + L"bmg.cmd", attributes);
}

static bool MoveProgramToAppData(std::wstring& message) {
    const std::wstring source = GetExecutableFilePath();
    const std::wstring target = GetBatchMenuGeneratorAppDataDir() + L"BatchMenuGenerator.exe";
    if (source.empty()) {
        message = L"Could not resolve the current program location.";
        return false;
    }
    if (NormalizePathForCompare(source) == NormalizePathForCompare(target)) {
        message = L"BatchMenuGenerator is already in its AppData folder.";
        return true;
    }

    const std::wstring temp = target + L".move." + std::to_wstring(GetTickCount64()) + L".tmp";
    if (!CopyFileW(source.c_str(), temp.c_str(), TRUE)) {
        message = L"Could not copy the running program into AppData.";
        return false;
    }
    if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp.c_str());
        message = L"Could not place the program in AppData. Close any other BMG copy and retry.";
        return false;
    }

    if (!UpdateShortcutIfPresent(true, target) || !UpdateShortcutIfPresent(false, target)) {
        message = L"The AppData copy was created, but an existing shortcut could not be updated. The original program remains in place.";
        return false;
    }
    if (BmgCommandsAreInstalled()) {
        std::wstring updateMessage;
        if (!InstallOrUpdateBmgCommands(target, updateMessage)) {
            message = L"The AppData copy was created, but the existing BMG command link could not be updated. The original program remains in place.";
            return false;
        }
    }

    std::wstring command = QuoteCommandLineArgument(target) + L" --bmg-finish-move " +
        QuoteCommandLineArgument(source) + L" " + std::to_wstring(GetCurrentProcessId());
    std::vector<wchar_t> commandBuffer(command.begin(), command.end());
    commandBuffer.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const std::wstring targetDirectory = GetExecutableDirectory(target);
    if (!CreateProcessW(target.c_str(), commandBuffer.data(), NULL, NULL, TRUE, 0, NULL,
        targetDirectory.empty() ? NULL : targetDirectory.c_str(), &startup, &process)) {
        message = L"The AppData copy was created, but Windows could not start it. The original program remains in place.";
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    g_exitAfterMove = true;
    message = L"Starting BatchMenuGenerator from AppData.";
    return true;
}

static void ShowProgramSetupTips() {
    system("cls");
    C_H; wprintf(L"  Program Setup Tips\n\n"); C_R;
    C_T; wprintf(L"  BatchMenuGenerator is portable. If you move it after creating shortcuts or\n");
    wprintf(L"  installing symlinks, they can break, leave leftover data, and stop working.\n");
    wprintf(L"  Keep the program in one location.\n\n"); C_R;
    C_P; wprintf(L"  Press any key to return..."); C_R;
    _getwch();
}

static void ShowProgramSetupMessage(const std::wstring& message) {
    system("cls");
    C_S; wprintf(L"  %ls\n\n", message.c_str()); C_R;
    C_P; wprintf(L"  Press any key to return..."); C_R;
    _getwch();
}

static void RunProgramSetupMenu() {
    const std::vector<std::wstring> actions = {
        L"Move program to AppData",
        L"Create desktop shortcut",
        L"Add program to Start menu (Search)",
        L"Install BMG commands (symlinks)",
        L"Update BMG commands (symlinks)",
        L"Uninstall BMG commands (symlinks)",
        L"Tips",
        L"Back"
    };
    while (true) {
        int choice = -1;
        if (g_enableTUI) {
            choice = RunTUIMenu(L"Set Up Program", actions);
        } else {
            system("cls");
            C_B; TuiBannerBorder();
            C_N; TuiBannerTitle(L"SET UP PROGRAM");
            C_B; TuiBannerBorder();
            wprintf(L"\n"); C_R;
            for (size_t i = 0; i + 1 < actions.size(); ++i)
                wprintf(L"    %zu) %ls\n", i + 1, actions[i].c_str());
            C_P; wprintf(L"    0) Back\n\n  Select option: "); C_I;
            int typed = _wtoi(ReadLine().c_str()); C_R;
            if (typed == 0) return;
            choice = typed - 1;
        }
        if (choice < 0 || choice == 7) return;

        if (choice == 6) {
            ShowProgramSetupTips();
            continue;
        }

        std::wstring message;
        bool success = false;
        if (choice == 0) success = MoveProgramToAppData(message);
        else if (choice == 1) {
            success = CreateProgramShortcut(true);
            message = success ? L"Desktop shortcut created." : L"Could not create the desktop shortcut.";
        } else if (choice == 2) {
            success = CreateProgramShortcut(false);
            message = success ? L"Start menu shortcut created. Search for BatchMenuGenerator." : L"Could not create the Start menu shortcut.";
        } else if (choice == 3 || choice == 4) {
            success = InstallOrUpdateBmgCommands(GetExecutableFilePath(), message);
        } else if (choice == 5) {
            success = UninstallBmgCommands(message);
        }

        if (g_exitAfterMove) return;
        if (message.empty()) message = success ? L"Setup action completed." : L"Setup action failed.";
        ShowProgramSetupMessage(message);
    }
}

static void RunSettingsMenu() {
    const AppConfig originalConfig = g_appConfig;
    const bool originalTUI = g_enableTUI;
    const bool originalSound = g_enableSound;
    bool settingsSaved = false;

    if (g_enableTUI) {
        int cursor = 0;
        while (true) {
            std::wstring endStr[] = { L"0 [Prompt]", L"1 [Main Menu]", L"2 [Launch Script]", L"3 [Exit]" };
            std::wstring saveloadStr[] = { L"0 [Prompt]", L"1 [Explorer]", L"2 [Manual Path]" };
            std::wstring borderStr[] = { L"0 [Prompt]", L"1 [Classic =]", L"2 [Modern -]", L"3 [ZigZag Z]" };

            std::vector<std::wstring> opts;
            std::vector<int> mapping;

            opts.push_back(L"Theme Engine : " + std::wstring(g_appConfig.theme_engine ? L"1 [Enabled]" : L"0 [Disabled]"));
            mapping.push_back(0);

            if (g_appConfig.theme_engine) {
                opts.push_back(L"Theme Preset : " + g_appConfig.theme_preset);
                mapping.push_back(1);
            }

            opts.push_back(L"AutoSave     : " + std::wstring(g_appConfig.autosave ? L"1 [Enabled]" : L"0 [Disabled]")); mapping.push_back(2);
            opts.push_back(L"TUI Mode     : " + std::wstring(g_enableTUI ? L"1 [Enabled]" : L"0 [Disabled]")); mapping.push_back(3);
            opts.push_back(L"Menu Sounds  : " + std::wstring(g_enableSound ? L"1 [Enabled]" : L"0 [Disabled]")); mapping.push_back(4);
            opts.push_back(L"End Action   : " + endStr[g_appConfig.end]); mapping.push_back(5);
            opts.push_back(L"Saveload     : " + saveloadStr[g_appConfig.saveload]); mapping.push_back(6);
            opts.push_back(L"Menuborder   : " + borderStr[g_appConfig.menuborder]); mapping.push_back(7);
            opts.push_back(L"View Version History"); mapping.push_back(8);
            opts.push_back(L"Set Up Program"); mapping.push_back(9);
            opts.push_back(L"[ Save & Exit ]"); mapping.push_back(10);
            opts.push_back(L"[ Cancel ]"); mapping.push_back(11);

            cursor = RunTUIMenu(L"SETTINGS (bconfig)", opts, cursor);
            if (cursor == -1) break;

            int action = mapping[(size_t)cursor];

            if (action == 11) break;
            else if (action == 0) {
                g_appConfig.theme_engine = !g_appConfig.theme_engine;
                if (!g_appConfig.theme_engine) ApplyTheme(L"default");
                else ApplyTheme(g_appConfig.theme_preset);
            } else if (action == 1) {
                const wchar_t* themes[] = { L"default", L"matrix", L"cyberpunk", L"cli", L"violent" };
                for (int i = 0; i < 5; ++i) {
                    if (g_appConfig.theme_preset == themes[i]) {
                        g_appConfig.theme_preset = themes[(i + 1) % 5];
                        ApplyTheme(g_appConfig.theme_preset);
                        break;
                    }
                }
            } else if (action == 2) {
                g_appConfig.autosave = !g_appConfig.autosave;
            } else if (action == 3) {
                g_enableTUI = !g_enableTUI;
                g_appConfig.tui = g_enableTUI ? 1 : 0;
            } else if (action == 4) {
                g_enableSound = !g_enableSound;
                g_appConfig.sound = g_enableSound ? 1 : 0;
            } else if (action == 5) {
                g_appConfig.end = (g_appConfig.end + 1) % 4;
            } else if (action == 6) {
                g_appConfig.saveload = (g_appConfig.saveload + 1) % 3;
            } else if (action == 7) {
                g_appConfig.menuborder = (g_appConfig.menuborder + 1) % 4;
            } else if (action == 8) {
                RunVersionHistoryMenu();
            } else if (action == 9) {
                RunProgramSetupMenu();
                if (g_exitAfterMove) return;
            } else if (action == 10) {
                if (SaveSettingsFile(g_appConfig)) {
                    settingsSaved = true;
                    C_S; wprintf(L"\n\n  [SUCCESS] Settings saved!\n"); C_R;
                    Sleep(800);
                    break;
                } else {
                    C_E; wprintf(L"\n\n  [ERROR] Could not save settings to disk. Changes were not committed.\n"); C_R;
                    Sleep(1000);
                }
            }
        }
    } else {
        while (true) {
            system("cls");
            C_B; TuiBannerBorder();
            C_N; TuiBannerTitle(L"SETTINGS (bconfig)");
            C_B; TuiBannerBorder();
            wprintf(L"\n"); C_T;

            std::wstring endStr[] = { L"0 [Prompt question]", L"1 [Main menu]", L"2 [Launch script]", L"3 [Exit]" };
            std::wstring saveloadStr[] = { L"0 [Prompt question]", L"1 [File Explorer]", L"2 [Manual path]" };
            std::wstring borderStr[] = { L"0 [Prompt question]", L"1 [Classic =]", L"2 [Modern -]", L"3 [ZigZag Z]" };

            C_P; wprintf(L"    1) Theme Engine : "); C_I; wprintf(L"%s\n", g_appConfig.theme_engine ? L"1 [Enabled]" : L"0 [Disabled]");
            if (g_appConfig.theme_engine) {
                C_P; wprintf(L"    2) Theme Preset : "); C_I; wprintf(L"%ls\n", g_appConfig.theme_preset.c_str());
            } else {
                C_T; wprintf(L"       (Theme Preset hidden while Theme Engine is disabled)\n");
            }
            C_P; wprintf(L"    3) AutoSave     : "); C_I; wprintf(L"%s\n", g_appConfig.autosave ? L"1 [Enabled]" : L"0 [Disabled]");
            C_P; wprintf(L"    4) TUI Mode     : "); C_I; wprintf(L"%s\n", g_enableTUI ? L"1 [Enabled]" : L"0 [Disabled]");
            C_P; wprintf(L"    5) Menu Sounds  : "); C_I; wprintf(L"%s\n", g_enableSound ? L"1 [Enabled]" : L"0 [Disabled]");
            C_P; wprintf(L"    6) End Action   : "); C_I; wprintf(L"%ls\n", endStr[g_appConfig.end].c_str());
            C_P; wprintf(L"    7) Saveload     : "); C_I; wprintf(L"%ls\n", saveloadStr[g_appConfig.saveload].c_str());
            C_P; wprintf(L"    8) Menuborder   : "); C_I; wprintf(L"%ls\n", borderStr[g_appConfig.menuborder].c_str());

            C_P; wprintf(L"\n    P) Set Up Program\n");
            C_P; wprintf(L"\n    V) View Version History\n");
            C_P; wprintf(L"\n    S) Save & Exit\n    0) Cancel\n\n  Select option: "); C_I;
            int ch = _getwch(); C_T;

            if (ch == '1') {
                g_appConfig.theme_engine = !g_appConfig.theme_engine;
                if (!g_appConfig.theme_engine) ApplyTheme(L"default");
                else ApplyTheme(g_appConfig.theme_preset);
            } else if (ch == '2' && g_appConfig.theme_engine) {
                const wchar_t* themes[] = { L"default", L"matrix", L"cyberpunk", L"cli", L"violent" };
                for (int i = 0; i < 5; ++i) {
                    if (g_appConfig.theme_preset == themes[i]) {
                        g_appConfig.theme_preset = themes[(i + 1) % 5];
                        ApplyTheme(g_appConfig.theme_preset);
                        break;
                    }
                }
            } else if (ch == '3') {
                g_appConfig.autosave = !g_appConfig.autosave;
            } else if (ch == '4') {
                g_enableTUI = !g_enableTUI;
                g_appConfig.tui = g_enableTUI ? 1 : 0;
            } else if (ch == '5') {
                g_enableSound = !g_enableSound;
                g_appConfig.sound = g_enableSound ? 1 : 0;
            } else if (ch == '6') {
                g_appConfig.end = (g_appConfig.end + 1) % 4;
            } else if (ch == '7') {
                g_appConfig.saveload = (g_appConfig.saveload + 1) % 3;
            } else if (ch == '8') {
                g_appConfig.menuborder = (g_appConfig.menuborder + 1) % 4;
            } else if (ch == 'V' || ch == 'v') {
                RunVersionHistoryMenu();
            } else if (ch == 'P' || ch == 'p') {
                RunProgramSetupMenu();
                if (g_exitAfterMove) return;
            } else if (ch == 'S' || ch == 's') {
                if (SaveSettingsFile(g_appConfig)) {
                    settingsSaved = true;
                    C_S; wprintf(L"\n\n  [SUCCESS] Settings saved to disk!\n"); C_R;
                    Sleep(1000);
                    break;
                } else {
                    C_E; wprintf(L"\n\n  [ERROR] Could not save settings to disk. Changes were not committed.\n"); C_R;
                    Sleep(1200);
                }
            } else if (ch == '0' || ch == 27) {
                break;
            }
        }
    }

    if (!settingsSaved) {
        g_appConfig = originalConfig;
        g_enableTUI = originalTUI;
        g_enableSound = originalSound;
        if (g_appConfig.theme_engine) ApplyTheme(g_appConfig.theme_preset);
        else ApplyTheme(L"default");
    }

    system("cls");
}

static std::wstring GetAppDataDir() {
    return GetBatchMenuGeneratorAppDataDir();
}

static std::wstring GetAutoSavePath() {
    return GetAppDataDir() + L"autosave.txt";
}

static std::wstring ReadAutoSaveTimestamp(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) return L"";
    FILE* f = NULL;
    if (_wfopen_s(&f, path.c_str(), L"r") != 0 || f == NULL) return L"unknown date";
    std::ifstream file(f);
    if (!file.is_open()) { fclose(f); return L"unknown date"; }
    std::string line;
    if (std::getline(file, line)) {
        std::wstring wline = ConvertToWide(line);
        size_t pos = wline.find(L"// Autosave from ");
        if (pos != std::wstring::npos) {
            std::wstring ts = Trim(wline.substr(16));
            file.close();
            return ts;
        }
    }
    file.close();
    return L"unknown date";
}

static void WriteAutoSave(const std::vector<std::wstring>& lines) {
    std::wstring path = GetAutoSavePath();
    HANDLE hf = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return;
    DWORD wr;
    BYTE bom[] = { 0xEF, 0xBB, 0xBF };
    WriteFile(hf, bom, 3, &wr, NULL);
    for (const auto& line : lines) {
        int len = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), NULL, 0, NULL, NULL);
        if (len > 0) {
            std::vector<char> utf8Buf(len);
            WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), utf8Buf.data(), len, NULL, NULL);
            WriteFile(hf, utf8Buf.data(), len, &wr, NULL);
        }
        WriteFile(hf, "\r\n", 2, &wr, NULL);
    }
    CloseHandle(hf);
}

static std::vector<std::wstring> ReadAutoSave(const std::wstring& path) {
    std::vector<std::wstring> result;
    if (path.empty()) return result;
    FILE* f = NULL;
    if (_wfopen_s(&f, path.c_str(), L"r") != 0 || f == NULL) return result;
    std::ifstream file(f);
    if (!file.is_open()) { fclose(f); return result; }
    std::string line;
    while (std::getline(file, line)) {
        result.push_back(ConvertToWide(line));
    }
    file.close();
    return result;
}

static void DeleteAutoSave(const std::wstring& path) {
    if (!path.empty()) DeleteFileW(path.c_str());
}

static std::wstring MakeBorder(wchar_t ch) {
    return std::wstring(57, ch);
}

static std::wstring GenerateBatch(const std::wstring& title,
    const std::vector<std::wstring>& description,
    const std::vector<OptionData>& options,
    const std::wstring& password,
    bool requireAdmin,
    wchar_t borderChar) {

    std::wstring border = MakeBorder(borderChar);
    if (border.empty()) border = MakeBorder(L'=');

    const std::wstring escapedBorder = EscapeBatch(border, false);
    const int BANNER_WIDTH = 57;

    std::wstring bat;
    bat.reserve(16384);
    if (requireAdmin) {
        bat += L":: Created with BatchMenuGenerator!\r\n";
    } else {
        bat += L"Created with BatchMenuGenerator!\r\n";
    }
    bat += L"@echo off\r\n";

    if (requireAdmin) {
        bat += L":: Admin check\r\n";
        bat += L"net session >nul 2>&1\r\n";
        bat += L"if %errorLevel% neq 0 (\r\n";
        bat += L"    echo " + escapedBorder + L"\r\n";
        bat += L"    echo  Requires administrator privileges!\r\n";
        bat += L"    echo  Relaunching as Admin...\r\n";
        bat += L"    echo " + escapedBorder + L"\r\n";
        bat += L"    timeout /t 5 >nul\r\n";
        bat += L"    powershell -Command \"Start-Process -FilePath '%~f0' -Verb RunAs -Wait\"\r\n";
        bat += L"    exit /b\r\n";
        bat += L")\r\n\r\n";
    }

    if (!password.empty()) {
        bat += L":auth_gate\r\ncls\r\n";
        bat += L"set /p \"pass_input=Enter password: \"\r\n";
        bat += L"if \"%pass_input%\"==\"" + EscapeBatch(password, false) + L"\" goto menu\r\n";
        bat += L"echo [!] Incorrect password.\r\npause\r\ngoto auth_gate\r\n\r\n";
    }

    if (!title.empty()) {
        bat += L"title " + EscapeBatch(title, false) + L"\r\n";
    }
    bat += L":menu\r\ncls\r\n";

    auto emitMenu = [&](auto&& self,
                        const std::vector<OptionData>& menuOptions,
                        const std::wstring& menuLabel,
                        const std::wstring& backLabel,
                        const std::wstring& idPrefix,
                        const std::wstring& displayHeading) -> void {
        bat += L":" + menuLabel + L"\r\n";
        bat += L"cls\r\n";
        bat += L"echo " + escapedBorder + L"\r\n";

        if (!displayHeading.empty()) {
            int headingLen = (int)displayHeading.length();
            int padLeft = (BANNER_WIDTH - headingLen) / 2;
            if (padLeft < 0) padLeft = 0;
            bat += L"echo " + std::wstring((size_t)padLeft, L' ') + EscapeBatch(displayHeading, false) + L"\r\n";
            bat += L"echo " + escapedBorder + L"\r\n";
        }

        bat += L"echo.\r\n";

        for (size_t i = 0; i < menuOptions.size(); ++i) {
            std::wstring label = menuOptions[i].label.empty()
                ? (L"Option " + std::to_wstring(i + 1))
                : menuOptions[i].label;
            std::wstring id = idPrefix.empty()
                ? std::to_wstring(i + 1)
                : idPrefix + L"_" + std::to_wstring(i + 1);

            bat += L"echo  " + std::to_wstring(i + 1) + L") " + EscapeBatch(label, false) + L"\r\n";
        }

        bat += L"echo  X) " + std::wstring(backLabel == L"close_menu" ? L"Exit" : L"Back") + L"\r\necho.\r\n";
        bat += L"echo " + escapedBorder + L"\r\necho.\r\n";

        bat += L"set \"choice=\"\r\n";
        bat += L"set /p choice=\"Select Option (1-" + std::to_wstring(menuOptions.size()) + L"): \"\r\n";
        for (size_t i = 0; i < menuOptions.size(); ++i) {
            std::wstring id = idPrefix.empty()
                ? std::to_wstring(i + 1)
                : idPrefix + L"_" + std::to_wstring(i + 1);
            bat += L"if /i \"%choice%\"==\"" + std::to_wstring(i + 1) + L"\" goto run_option" + id + L"\r\n";
        }
        bat += L"if /i \"%choice%\"==\"X\" goto " + backLabel + L"\r\n";
        bat += L"if /i \"%choice%\"==\"x\" goto " + backLabel + L"\r\n";
        bat += L"goto " + menuLabel + L"\r\n\r\n";

        for (size_t i = 0; i < menuOptions.size(); ++i) {
            const OptionData& opt = menuOptions[i];
            std::wstring id = idPrefix.empty()
                ? std::to_wstring(i + 1)
                : idPrefix + L"_" + std::to_wstring(i + 1);

            bat += L":run_option" + id + L"\r\n";
            bat += L"cls\r\n";

            if (opt.runMode == 2) {
                if (opt.subOptions.empty()) {
                    bat += L"echo No submenu options defined.\r\n";
                    bat += L"echo.\r\npause\r\ngoto " + menuLabel + L"\r\n\r\n";
                } else {
                    bat += L"goto submenu_" + id + L"\r\n\r\n";
                }
            } else {
                if (opt.commands.empty()) {
                    bat += L"echo No command specified.\r\n";
                } else {
                    for (const auto& rawCmd : opt.commands) {
                        std::wstring cmd = rawCmd;
                        if (cmd.length() >= 2 && cmd.front() == L'"' && cmd.back() == L'"')
                            cmd = cmd.substr(1, cmd.length() - 2);
                        if (cmd.empty()) continue;
                        if (opt.runMode == 1)
                            bat += L"cmd /c \"" + EscapeBatch(cmd, true) + L"\"\r\n";
                        else
                            bat += L"call " + EscapeBatch(cmd, false) + L"\r\n";
                    }
                }
                bat += L"echo.\r\npause\r\ngoto " + menuLabel + L"\r\n\r\n";
            }
        }

        for (size_t i = 0; i < menuOptions.size(); ++i) {
            const OptionData& opt = menuOptions[i];
            if (opt.runMode != 2 || opt.subOptions.empty()) continue;

            std::wstring id = idPrefix.empty()
                ? std::to_wstring(i + 1)
                : idPrefix + L"_" + std::to_wstring(i + 1);
            std::wstring subLabel = L"submenu_" + id;

            self(self, opt.subOptions, subLabel, menuLabel, id,
                 opt.label.empty() ? (L"Option " + std::to_wstring(i + 1)) : opt.label);
        }
    };

    bat += L"echo " + escapedBorder + L"\r\n";
    if (!title.empty()) {
        int titleLen = (int)title.length();
        int padLeft = (BANNER_WIDTH - titleLen) / 2;
        if (padLeft < 0) padLeft = 0;
        bat += L"echo " + std::wstring((size_t)padLeft, L' ') + EscapeBatch(title, false) + L"\r\n";
    }
    bat += L"echo " + escapedBorder + L"\r\necho.\r\n";

    if (!description.empty()) {
        for (const auto& line : description) {
            if (line.empty()) bat += L"echo.\r\n";
            else bat += L"echo  " + EscapeBatch(line, false) + L"\r\n";
        }
        bat += L"echo.\r\n";
    }

    bat += L"echo " + escapedBorder + L"\r\necho.\r\n";

    for (size_t i = 0; i < options.size(); ++i) {
        std::wstring label = options[i].label.empty()
            ? (L"Option " + std::to_wstring(i + 1))
            : options[i].label;
        bat += L"echo  " + std::to_wstring(i + 1) + L") " + EscapeBatch(label, false) + L"\r\n";
    }
    bat += L"echo  X) Exit\r\necho.\r\n";
    bat += L"echo " + escapedBorder + L"\r\necho.\r\n";
    bat += L"set \"choice=\"\r\n";
    bat += L"set /p choice=\"Select Option (1-" + std::to_wstring(options.size()) + L"): \"\r\n";
    for (size_t i = 0; i < options.size(); ++i) {
        bat += L"if /i \"%choice%\"==\"" + std::to_wstring(i + 1) + L"\" goto run_option" + std::to_wstring(i + 1) + L"\r\n";
    }
    bat += L"if /i \"%choice%\"==\"X\" goto close_menu\r\n";
    bat += L"if /i \"%choice%\"==\"x\" goto close_menu\r\n";
    bat += L"goto menu\r\n\r\n";

    for (size_t i = 0; i < options.size(); ++i) {
        const OptionData& opt = options[i];
        std::wstring id = std::to_wstring(i + 1);

        bat += L":run_option" + id + L"\r\n";
        bat += L"cls\r\n";
        if (opt.runMode == 2) {
            if (opt.subOptions.empty()) {
                bat += L"echo No submenu options defined.\r\n";
                bat += L"echo.\r\npause\r\ngoto menu\r\n\r\n";
            } else {
                bat += L"goto submenu_" + id + L"\r\n\r\n";
            }
        } else {
            if (opt.commands.empty()) {
                bat += L"echo No command specified.\r\n";
            } else {
                for (const auto& rawCmd : opt.commands) {
                    std::wstring cmd = rawCmd;
                    if (cmd.length() >= 2 && cmd.front() == L'"' && cmd.back() == L'"')
                        cmd = cmd.substr(1, cmd.length() - 2);
                    if (cmd.empty()) continue;
                    if (opt.runMode == 1)
                        bat += L"cmd /c \"" + EscapeBatch(cmd, true) + L"\"\r\n";
                    else
                        bat += L"call " + EscapeBatch(cmd, false) + L"\r\n";
                }
            }
            bat += L"echo.\r\npause\r\ngoto menu\r\n\r\n";
        }
    }

    for (size_t i = 0; i < options.size(); ++i) {
        const OptionData& opt = options[i];
        if (opt.runMode != 2 || opt.subOptions.empty()) continue;

        std::wstring id = std::to_wstring(i + 1);
        std::wstring subLabel = L"submenu_" + id;
        emitMenu(emitMenu,
            opt.subOptions,
            subLabel,
            L"menu",
            id,
            opt.label.empty() ? (L"Option " + std::to_wstring(i + 1)) : opt.label);
    }

    bat += L":close_menu\r\ncls\r\ntitle cmd\r\ngoto :eof\r\n";
    return bat;
}

static bool WriteScriptToFile(const std::wstring& path, const std::wstring& script, bool writeBom = true) {
    HANDLE hf = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return false;
    DWORD wr;
    if (writeBom) {
        BYTE bom[] = { 0xEF, 0xBB, 0xBF };
        WriteFile(hf, bom, 3, &wr, NULL);
    }
    int len = WideCharToMultiByte(CP_UTF8, 0, script.c_str(), (int)script.length(), NULL, 0, NULL, NULL);
    if (len > 0) {
        std::vector<char> u(len);
        WideCharToMultiByte(CP_UTF8, 0, script.c_str(), (int)script.length(), u.data(), len, NULL, NULL);
        WriteFile(hf, u.data(), len, &wr, NULL);
    }
    CloseHandle(hf);
    return true;
}

static std::wstring GenerateTempFilePath() {
    wchar_t tempDir[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempDir) == 0) {
        wcscpy_s(tempDir, MAX_PATH, L"C:\\Windows\\Temp\\");
    }
    SYSTEMTIME st;
    GetSystemTime(&st);
    wchar_t buf[MAX_PATH];
    swprintf(buf, MAX_PATH, L"%sBMGtempScript_%04d%02d%02d_%02d%02d%02d.bat",
        tempDir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buf);
}

static void DeleteTempFile(const std::wstring& path) {
    if (path.empty()) return;
    if (path.find(L"BMGtempScript_") != std::wstring::npos) {
        DeleteFileW(path.c_str());
    }
}

static bool SaveToManualPath(const std::wstring& script, std::wstring& outSavedPath, std::wstring forcedFile = L"", std::wstring forcedDir = L"", bool writeBom = true) {
    while (true) {
        std::wstring filename = forcedFile;
        if (filename.empty()) {
            C_P; wprintf(L"\n  Enter filename: "); C_I;
            filename = ReadLine();
            if (filename.length() >= 2 && filename.front() == L'"' && filename.back() == L'"')
                filename = filename.substr(1, filename.length() - 2);
            filename = Trim(filename);
            if (filename.empty()) { C_E; wprintf(L"  [Error] Filename cannot be empty.\n"); C_R; continue; }
            const std::wstring illegal = L"<>:\"/\\|?*";
            bool invalid = false;
            for (wchar_t c : filename) { if (illegal.find(c) != std::wstring::npos) { invalid = true; break; } }
            if (invalid) { C_E; wprintf(L"  [Error] Invalid filename.\n"); C_R; continue; }
        }
        if (filename.length() < 4 || _wcsicmp(filename.c_str() + filename.length() - 4, L".bat") != 0)
            filename += L".bat";
        std::wstring dir = forcedDir;
        if (dir.empty()) { C_P; wprintf(L"  Enter path [Press Enter for Current Directory]: "); C_I;
            dir = ReadLine();
            if (dir.length() >= 2 && dir.front() == L'"' && dir.back() == L'"') dir = dir.substr(1, dir.length() - 2);
            dir = Trim(dir);
        }
        if (dir.empty()) { wchar_t buffer[MAX_PATH]; if (GetCurrentDirectoryW(MAX_PATH, buffer)) dir = buffer; }
        if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/') dir += L"\\";
        std::wstring fullPath = dir + filename;
        std::wstring baseName = filename.substr(0, filename.length() - 4);
        std::wstring checkDir = dir;
        while (!checkDir.empty() && (checkDir.back() == L'\\' || checkDir.back() == L'/')) {
            if (checkDir.length() == 3 && checkDir[1] == L':') break;
            checkDir.pop_back();
        }
        if (!checkDir.empty()) {
            DWORD attr = GetFileAttributesW(checkDir.c_str());
            if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                if (!forcedDir.empty()) {
                    std::wstring normalizedDir = checkDir;
                    for (auto& ch : normalizedDir) if (ch == L'/') ch = L'\\';
                    size_t pos = 0;
                    while ((pos = normalizedDir.find(L'\\', pos)) != std::wstring::npos) {
                        std::wstring sub = normalizedDir.substr(0, pos);
                        if (!sub.empty() && sub.back() != L':') CreateDirectoryW(sub.c_str(), NULL);
                        pos++;
                    }
                    CreateDirectoryW(normalizedDir.c_str(), NULL);
                } else {
                    C_E; wprintf(L"  [Error] Destination folder does not exist!\n"); C_P;

                    if (g_enableTUI) {
                        int r = RunTUIMenu(L"Destination folder does not exist!", {L"Retype file details", L"Create folder automatically", L"Exit & Save"});
                        if (r == 2 || r == -1) exit(0);
                        if (r == 1) {
                            std::wstring normalizedDir = checkDir;
                            for (auto& ch : normalizedDir) if (ch == L'/') ch = L'\\';
                            size_t pos = 0;
                            while ((pos = normalizedDir.find(L'\\', pos)) != std::wstring::npos) {
                                std::wstring sub = normalizedDir.substr(0, pos);
                                if (!sub.empty() && sub.back() != L':') CreateDirectoryW(sub.c_str(), NULL);
                                pos++;
                            }
                            CreateDirectoryW(normalizedDir.c_str(), NULL);
                            attr = GetFileAttributesW(checkDir.c_str());
                            if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                                C_E; wprintf(L"  [Error] Failed to create directory.\n"); C_R; continue;
                            }
                        } else { continue; }
                    } else {
                        wprintf(L"    1) Retype file details\n    2) Create folder automatically\n    X) Exit & Save\n  Select option [1]: "); C_I;
                        std::wstring opt = ReadLine();
                        if (opt == L"X" || opt == L"x") exit(0);
                        if (opt == L"2") {
                            std::wstring normalizedDir = checkDir;
                            for (auto& ch : normalizedDir) if (ch == L'/') ch = L'\\';
                            size_t pos = 0;
                            while ((pos = normalizedDir.find(L'\\', pos)) != std::wstring::npos) {
                                std::wstring sub = normalizedDir.substr(0, pos);
                                if (!sub.empty() && sub.back() != L':') CreateDirectoryW(sub.c_str(), NULL);
                                pos++;
                            }
                            CreateDirectoryW(normalizedDir.c_str(), NULL);
                            attr = GetFileAttributesW(checkDir.c_str());
                            if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                                C_E; wprintf(L"  [Error] Failed to create directory.\n"); C_R; continue;
                            }
                        } else { continue; }
                    }
                }
            }
        }
        if (forcedFile.empty()) {
            DWORD fileAttr = GetFileAttributesW(fullPath.c_str());
            if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                C_E; wprintf(L"  [Warning] File already exists!\n"); C_P;

                if (g_enableTUI) {
                    wchar_t buf[256]; swprintf(buf, 256, L"Rename to %ls1.bat", baseName.c_str());
                    int r = RunTUIMenu(L"File already exists!", {L"Replace", std::wstring(buf), L"Retype", L"Exit & Save"});
                    if (r == 3 || r == -1) exit(0);
                    if (r == 1) fullPath = dir + baseName + L"1.bat";
                    else if (r == 2) continue;
                } else {
                    wprintf(L"    1) Replace\n    2) Rename to %ls1.bat\n    3) Retype\n    X) Exit & Save\n  Select [1]: ", baseName.c_str()); C_I;
                    std::wstring opt = ReadLine();
                    if (opt == L"X" || opt == L"x") exit(0);
                    if (opt == L"2") fullPath = dir + baseName + L"1.bat";
                    else if (opt == L"3") continue;
                }
            }
        }
        if (WriteScriptToFile(fullPath, script, writeBom)) {
            C_S; wprintf(L"  [SUCCESS] Saved to: %ls\n", fullPath.c_str()); C_R;
            outSavedPath = fullPath; return true;
        } else {
            DWORD err = GetLastError();
            C_E; wprintf(L"  [ERROR] Failed to save. (Code: %lu)\n", err); C_R;
            if (forcedFile.empty()) {
                if (g_enableTUI) {
                    int r = RunTUIMenu(L"Failed to save script", {L"Retry", L"Use Explorer dialog", L"Exit & Save"}, 1);
                    if (r == 2 || r == -1) exit(0);
                    if (r == 1) {
                        wchar_t path[MAX_PATH] = {0};
                        OPENFILENAMEW of; ZeroMemory(&of, sizeof(of));
                        of.lStructSize = sizeof(of); of.lpstrFilter = L"Batch Files (*.bat)\0*.bat\0All Files (*.*)\0*.*\0\0";
                        of.lpstrFile = path; of.nMaxFile = MAX_PATH; of.lpstrDefExt = L"bat";
                        of.lpstrTitle = L"Save Batch Menu Script"; of.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
                        if (GetSaveFileNameW(&of) && WriteScriptToFile(path, script, writeBom)) {
                            C_S; wprintf(L"  [SUCCESS] Saved to: %ls\n", path); C_R;
                            outSavedPath = path; return true;
                        }
                        C_E; wprintf(L"  [Error] Explorer save failed.\n");
                    }
                } else {
                    C_P; wprintf(L"    1) Retry\n    2) Use Explorer dialog\n    X) Exit & Save\n  Select [2]: "); C_I;
                    std::wstring fc = ReadLine();
                    if (fc == L"X" || fc == L"x") exit(0);
                    if (fc.empty()) fc = L"2";
                    if (fc == L"2") {
                        wchar_t path[MAX_PATH] = {0};
                        OPENFILENAMEW of; ZeroMemory(&of, sizeof(of));
                        of.lStructSize = sizeof(of); of.lpstrFilter = L"Batch Files (*.bat)\0*.bat\0All Files (*.*)\0*.*\0\0";
                        of.lpstrFile = path; of.nMaxFile = MAX_PATH; of.lpstrDefExt = L"bat";
                        of.lpstrTitle = L"Save Batch Menu Script"; of.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
                        if (GetSaveFileNameW(&of) && WriteScriptToFile(path, script, writeBom)) {
                            C_S; wprintf(L"  [SUCCESS] Saved to: %ls\n", path); C_R;
                            outSavedPath = path; return true;
                        }
                        C_E; wprintf(L"  [Error] Explorer save failed.\n");
                    }
                }
            } else return false;
        }
    }
}

static bool ReadBinaryFile(const std::wstring& path, std::vector<unsigned char>& data) {
    data.clear();
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size) || size.QuadPart < 0 || size.QuadPart > 100 * 1024 * 1024) {
        CloseHandle(h);
        return false;
    }

    data.resize((size_t)size.QuadPart);
    DWORD total = 0;
    while (total < data.size()) {
        DWORD chunk = 0;
        if (!ReadFile(h, data.data() + total, (DWORD)std::min<size_t>(1 << 20, data.size() - total), &chunk, NULL)) {
            CloseHandle(h);
            data.clear();
            return false;
        }
        if (chunk == 0) break;
        total += chunk;
    }
    CloseHandle(h);
    if (total != data.size()) {
        data.clear();
        return false;
    }
    return true;
}

static std::string Base64Encode(const std::vector<unsigned char>& data) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    for (size_t i = 0; i < data.size(); i += 3) {
        unsigned int a = data[i];
        unsigned int b = (i + 1 < data.size()) ? data[i + 1] : 0;
        unsigned int c = (i + 2 < data.size()) ? data[i + 2] : 0;
        unsigned int triple = (a << 16) | (b << 8) | c;
        out.push_back(table[(triple >> 18) & 0x3F]);
        out.push_back(table[(triple >> 12) & 0x3F]);
        out.push_back((i + 1 < data.size()) ? table[(triple >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < data.size()) ? table[triple & 0x3F] : '=');
    }
    return out;
}

static std::wstring QuoteWindowsArg(const std::wstring& s) {
    if (s.empty()) return L"\"\"";
    std::wstring out = L"\"";
    size_t slashes = 0;
    for (wchar_t c : s) {
        if (c == L'\\') {
            ++slashes;
        } else if (c == L'"') {
            out.append(slashes * 2 + 1, L'\\');
            out += L'"';
            slashes = 0;
        } else {
            if (slashes) out.append(slashes, L'\\');
            slashes = 0;
            out += c;
        }
    }
    if (slashes) out.append(slashes * 2, L'\\');
    out += L'"';
    return out;
}

static bool RunHiddenProcess(const std::wstring& exePath,
    const std::vector<std::wstring>& args, DWORD& exitCode) {

    std::wstring cmdLine = QuoteWindowsArg(exePath);
    for (const auto& arg : args) {
        cmdLine += L" ";
        cmdLine += QuoteWindowsArg(arg);
    }

    std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
    mutableCmd.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};

    if (!CreateProcessW(NULL, mutableCmd.data(), NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

static std::wstring FindPowerShellExecutable() {
    wchar_t path[MAX_PATH] = {0};
    if (SearchPathW(NULL, L"powershell.exe", NULL, MAX_PATH, path, NULL) > 0)
        return path;
    return L"";
}

static std::wstring FindCscExecutable() {
    wchar_t path[MAX_PATH] = {0};
    if (SearchPathW(NULL, L"csc.exe", NULL, MAX_PATH, path, NULL) > 0)
        return path;

    wchar_t windir[MAX_PATH] = {0};
    if (GetEnvironmentVariableW(L"WINDIR", windir, MAX_PATH) > 0) {
        const wchar_t* relative[] = {
            L"\\Microsoft.NET\\Framework64\\v4.0.30319\\csc.exe",
            L"\\Microsoft.NET\\Framework\\v4.0.30319\\csc.exe"
        };
        for (const wchar_t* rel : relative) {
            std::wstring candidate = std::wstring(windir) + rel;
            if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES)
                return candidate;
        }
    }
    return L"";
}

static std::wstring GetPathDirectory(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L".";
    return path.substr(0, pos);
}

static std::wstring GetPathBaseName(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    size_t dot = path.find_last_of(L'.');
    size_t start = (slash == std::wstring::npos) ? 0 : slash + 1;
    if (dot == std::wstring::npos || dot < start) dot = path.length();
    return path.substr(start, dot - start);
}

static bool ChooseExeOutputPath(const std::wstring& sourcePath, std::wstring& outputPath) {
    std::wstring dir = GetPathDirectory(sourcePath);
    std::wstring base = GetPathBaseName(sourcePath);
    std::wstring defaultPath = dir + L"\\" + base + L".exe";

    wchar_t path[MAX_PATH] = {0};
    wcsncpy_s(path, defaultPath.c_str(), _TRUNCATE);

    OPENFILENAMEW of{};
    of.lStructSize = sizeof(of);
    of.lpstrFilter = L"Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0\0";
    of.lpstrFile = path;
    of.nMaxFile = MAX_PATH;
    of.lpstrDefExt = L"exe";
    of.lpstrTitle = L"Compile Batch Script to EXE";
    of.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

    if (!GetSaveFileNameW(&of)) return false;
    outputPath = path;
    if (outputPath.length() < 4 || _wcsicmp(outputPath.c_str() + outputPath.length() - 4, L".exe") != 0)
        outputPath += L".exe";
    return true;
}

static bool CompileBatchToExe(const std::wstring& sourceBat, const std::wstring& outputExe) {
    std::vector<unsigned char> plain;
    if (!ReadBinaryFile(sourceBat, plain)) {
        C_E; wprintf(L"  [Error] Could not read batch script: %ls\n", sourceBat.c_str()); C_R;
        return false;
    }

    if (plain.empty()) {
        C_E; wprintf(L"  [Error] Batch script is empty.\n"); C_R;
        return false;
    }

    DWORD attr = GetFileAttributesW(outputExe.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        C_E; wprintf(L"  [Error] Output path is a directory.\n"); C_R;
        return false;
    }

    unsigned char key[16] = {0};
    LARGE_INTEGER qpc{};
    QueryPerformanceCounter(&qpc);
    ULONGLONG seed = GetTickCount64() ^ (ULONGLONG)GetCurrentProcessId() ^
        (ULONGLONG)GetCurrentThreadId() ^ (ULONGLONG)qpc.QuadPart;
    for (int i = 0; i < 16; ++i) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        key[i] = (unsigned char)((seed >> ((i % 8) * 8)) & 0xFF);
    }

    std::vector<unsigned char> cipher = plain;
    for (size_t i = 0; i < cipher.size(); ++i) {
        unsigned char mix = (unsigned char)((i * 29u + 0x5Au) & 0xFFu);
        cipher[i] = (unsigned char)(cipher[i] ^ key[i % 16] ^ mix);
    }

    std::string payloadB64 = Base64Encode(cipher);
    std::string keyB64;
    {
        std::vector<unsigned char> keyVec(key, key + 16);
        keyB64 = Base64Encode(keyVec);
    }

    std::wstring tempBase = GetAppDataDir() + L"bmg_compile_" +
        std::to_wstring(GetCurrentProcessId()) + L"_" +
        std::to_wstring(GetTickCount64()) + L"_";

    std::wstring csPath = tempBase + L"wrapper.cs";
    std::wstring psPath = tempBase + L"compile.ps1";
    std::wstring logPath = tempBase + L"compile.log";

    std::string cs =
R"(using System;
using System.Diagnostics;
using System.IO;
using System.Text;

public static class Program
{
    static readonly string Payload = ")" + payloadB64 + R"(";
    static readonly string KeyB64 = ")" + keyB64 + R"(";

    static byte[] DecodePayload()
    {
        byte[] enc = Convert.FromBase64String(Payload);
        byte[] key = Convert.FromBase64String(KeyB64);
        for (int i = 0; i < enc.Length; i++)
        {
            byte mix = (byte)((i * 29 + 0x5A) & 0xFF);
            enc[i] = (byte)(enc[i] ^ key[i % key.Length] ^ mix);
        }
        return enc;
    }

    static string QuoteArg(string s)
    {
        if (s.Length == 0) return "\"\"";
        StringBuilder b = new StringBuilder("\"");
        int slashes = 0;
        foreach (char c in s)
        {
            if (c == '\\')
            {
                slashes++;
            }
            else if (c == '"')
            {
                b.Append('\\', slashes * 2 + 1);
                b.Append('"');
                slashes = 0;
            }
            else
            {
                if (slashes > 0) b.Append('\\', slashes);
                slashes = 0;
                b.Append(c);
            }
        }
        if (slashes > 0) b.Append('\\', slashes * 2);
        b.Append('"');
        return b.ToString();
    }

    public static int Main(string[] args)
    {
        string temp = Path.Combine(Path.GetTempPath(),
            "BMG_" + Guid.NewGuid().ToString("N") + ".cmd");

        int exitCode = 1;
        try
        {
            File.WriteAllBytes(temp, DecodePayload());

            string comspec = Environment.GetEnvironmentVariable("ComSpec");
            if (String.IsNullOrEmpty(comspec)) comspec = "cmd.exe";

            StringBuilder command = new StringBuilder("/d /c call ");
            command.Append(QuoteArg(temp));
            for (int i = 0; i < args.Length; i++)
            {
                command.Append(' ');
                command.Append(QuoteArg(args[i]));
            }

            ProcessStartInfo psi = new ProcessStartInfo();
            psi.FileName = comspec;
            psi.Arguments = command.ToString();
            psi.WorkingDirectory = AppDomain.CurrentDomain.BaseDirectory.TrimEnd('\\');
            psi.UseShellExecute = false;
            psi.CreateNoWindow = false;

            using (Process proc = Process.Start(psi))
            {
                proc.WaitForExit();
                exitCode = proc.ExitCode;
            }
        }
        catch (Exception ex)
        {
            try
            {
                Console.Error.WriteLine(ex.ToString());
            }
            catch { }
            exitCode = 1;
        }
        finally
        {
            try { File.Delete(temp); } catch { }
        }

        return exitCode;
    }
}
)";

    std::wstring csWide = ConvertToWide(cs);
    if (!WriteScriptToFile(csPath, csWide, false)) {
        C_E; wprintf(L"  [Error] Failed to create compiler source.\n"); C_R;
        DeleteFileW(csPath.c_str());
        return false;
    }

    if (GetFileAttributesW(outputExe.c_str()) != INVALID_FILE_ATTRIBUTES)
        DeleteFileW(outputExe.c_str());

    C_N; wprintf(L"  Compiling EXE...\n"); C_R;

    bool launched = false;
    DWORD exitCode = 1;
    std::wstring csc = FindCscExecutable();

    if (!csc.empty()) {
        launched = RunHiddenProcess(csc, {
            L"/nologo",
            L"/target:exe",
            L"/optimize+",
            L"/out:" + outputExe,
            csPath
        }, exitCode);
    } else {
        std::wstring ps1 =
            L"$ErrorActionPreference = 'Stop'\r\n"
            L"try {\r\n"
            L"  $source = [System.IO.File]::ReadAllText($args[0])\r\n"
            L"  Add-Type -TypeDefinition $source -OutputAssembly $args[1] -OutputType ConsoleApplication\r\n"
            L"} catch {\r\n"
            L"  [System.IO.File]::WriteAllText($args[2], ($_ | Out-String))\r\n"
            L"  exit 1\r\n"
            L"}\r\n"
            L"exit 0\r\n";

        if (!WriteScriptToFile(psPath, ps1, true)) {
            C_E; wprintf(L"  [Error] Failed to create PowerShell compiler script.\n"); C_R;
            DeleteFileW(csPath.c_str());
            return false;
        }

        std::wstring ps = FindPowerShellExecutable();
        if (!ps.empty()) {
            launched = RunHiddenProcess(ps, {
                L"-NoProfile", L"-NonInteractive", L"-ExecutionPolicy", L"Bypass",
                L"-File", psPath, csPath, outputExe, logPath
            }, exitCode);
        }
    }

    bool ok = launched && exitCode == 0 &&
        GetFileAttributesW(outputExe.c_str()) != INVALID_FILE_ATTRIBUTES;

    if (!ok) {
        DeleteFileW(outputExe.c_str());
        C_E;
        wprintf(L"  [Error] EXE compilation failed.\n");
        C_R;

        std::vector<unsigned char> logBytes;
        if (ReadBinaryFile(logPath, logBytes) && !logBytes.empty()) {
            std::wstring log = ConvertToWide(std::string(logBytes.begin(), logBytes.end()));
            C_E; wprintf(L"%ls\n", log.c_str()); C_R;
        } else if (!launched) {
            DWORD err = GetLastError();
            C_E; wprintf(L"  [Error] No compatible C# compiler was available (or compiler process could not be launched), code %lu.\n", err); C_R;
        }
    } else {
        C_S; wprintf(L"  [SUCCESS] Compiled EXE: %ls\n", outputExe.c_str()); C_R;
    }

    DeleteFileW(csPath.c_str());
    DeleteFileW(psPath.c_str());
    DeleteFileW(logPath.c_str());
    return ok;
}

static bool CompileBatchToExeInteractive(const std::wstring& sourceBat) {
    std::wstring outputExe;
    if (!ChooseExeOutputPath(sourceBat, outputExe)) {
        C_P; wprintf(L"  EXE compilation canceled.\n"); C_R;
        return false;
    }
    return CompileBatchToExe(sourceBat, outputExe);
}

static bool IsBatchMenuGeneratorScript(const std::vector<std::wstring>& lines) {
    if (lines.empty()) return false;
    std::wstring firstLine = Trim(lines[0]);
    return (firstLine.find(L"Created with BatchMenuGenerator!") != std::wstring::npos);
}

static bool GetBatchEchoContent(const std::wstring& rawLine, std::wstring& outContent) {
    size_t p = rawLine.find_first_not_of(L" \t");
    if (p == std::wstring::npos) return false;
    if (rawLine.size() < p + 5 || rawLine.compare(p, 5, L"echo ") != 0) return false;
    outContent = rawLine.substr(p + 5);

    if (!outContent.empty() && outContent.front() == L' ')
        outContent.erase(outContent.begin());

    while (!outContent.empty() &&
           (outContent.back() == L'\r' || outContent.back() == L'\n')) {
        outContent.pop_back();
    }
    return true;
}

static bool ExtractQuotedBatchValue(const std::wstring& line, size_t startQuote, std::wstring& outValue) {
    if (startQuote == std::wstring::npos || startQuote >= line.size()) return false;
    std::wstring value;
    for (size_t i = startQuote + 1; i < line.size(); ++i) {
        if (line[i] == L'^' && i + 1 < line.size()) {
            value += line[i];
            value += line[++i];
            continue;
        }
        if (line[i] == L'"') {
            outValue = UnescapeBatch(value);
            return true;
        }
        value += line[i];
    }
    return false;
}

static bool IsEchoBorderLine(const std::wstring& line, wchar_t borderChar) {
    size_t first = line.find_first_not_of(L" \t");
    if (first == std::wstring::npos || line.size() <= first + 5 ||
        line.compare(first, 5, L"echo ") != 0 || line[first + 5] == L' ') return false;

    std::wstring content;
    if (!GetBatchEchoContent(line, content)) return false;
    content = UnescapeBatch(content);
    if (content.size() < 20) return false;
    for (wchar_t c : content) {
        if (c != borderChar) return false;
    }
    return true;
}

static bool ParseGeneratedMenuBlock(const std::vector<std::wstring>& lines,
    const std::wstring& menuLabel,
    const std::wstring& backLabel,
    const std::wstring& idPrefix,
    std::vector<OptionData>& outOptions,
    int depth = 0) {

    const int MAX_PARSE_DEPTH = 8;
    if (depth > MAX_PARSE_DEPTH) return false;

    size_t menuIdx = std::wstring::npos;
    std::wstring target = L":" + menuLabel;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (Trim(lines[i]) == target) {
            menuIdx = i;
            break;
        }
    }
    if (menuIdx == std::wstring::npos) return false;

    std::vector<std::pair<int, std::wstring>> found;
    size_t choiceIdx = std::wstring::npos;

    for (size_t i = menuIdx + 1; i < lines.size(); ++i) {
        std::wstring tr = Trim(lines[i]);
        if (tr.find(L"set /p choice=") == 0) {
            choiceIdx = i;
            break;
        }

        if (tr.size() >= 5 && tr.substr(0, 5) == L"echo ") {
            std::wstring content;
            if (!GetBatchEchoContent(lines[i], content)) continue;
            content = UnescapeBatch(content);

            size_t paren = content.find(L") ");
            if (paren != std::wstring::npos) {
                std::wstring numPart = Trim(content.substr(0, paren));
                bool numeric = !numPart.empty();
                for (wchar_t ch : numPart) if (!iswdigit(ch)) numeric = false;
                if (numeric) {
                    int num = _wtoi(numPart.c_str());
                    if (num > 0 && num <= 20) {
                        found.push_back({ num, content.substr(paren + 2) });
                    }
                }
            }
        }

        if (tr == L":close_menu" || tr.find(L":submenu_") == 0 || tr.find(L":run_option") == 0)
            break;
    }

    if (choiceIdx == std::wstring::npos) return false;

    std::sort(found.begin(), found.end(),
        [](const std::pair<int, std::wstring>& a, const std::pair<int, std::wstring>& b) {
            return a.first < b.first;
        });

    outOptions.clear();
    outOptions.resize(found.size());
    for (size_t i = 0; i < found.size(); ++i) {
        outOptions[i].label = found[i].second;
    }

    for (size_t i = 0; i < outOptions.size(); ++i) {
        std::wstring id = idPrefix.empty()
            ? std::to_wstring(i + 1)
            : idPrefix + L"_" + std::to_wstring(i + 1);
        std::wstring runLabel = L":run_option" + id;

        size_t optStart = std::wstring::npos;
        for (size_t k = choiceIdx + 1; k < lines.size(); ++k) {
            std::wstring tr = Trim(lines[k]);
            if (tr == runLabel) {
                optStart = k;
                break;
            }
            if (tr == L":close_menu") break;
        }

        if (optStart == std::wstring::npos) continue;

        OptionData& opt = outOptions[i];
        opt.commands.clear();
        opt.subOptions.clear();
        opt.runMode = 0;

        bool hasSubmenu = false;
        std::vector<std::wstring> backendCommands;

        for (size_t k = optStart + 1; k < lines.size(); ++k) {
            std::wstring tr = Trim(lines[k]);
            if (tr.find(L":run_option") == 0 && tr != runLabel) break;
            if (tr == L":close_menu") break;

            if (tr.find(L"goto submenu_" + id) == 0) {
                hasSubmenu = true;
                break;
            }
            if (tr == L"cls" || tr == L"echo." || tr == L"pause" || tr.empty()) continue;
            if (tr == L":: BMG_RUNMODE:1") {
                opt.runMode = 1;
            } else if (tr.find(L"cmd /c ") == 0) {
                opt.runMode = 1;
                std::wstring cmd = Trim(tr.substr(7));
                if (cmd.size() >= 2 && cmd.front() == L'"' && cmd.back() == L'"')
                    cmd = cmd.substr(1, cmd.size() - 2);
                backendCommands.push_back(UnescapeBatch(cmd, true));
            } else if (tr.find(L"call ") == 0) {
                backendCommands.push_back(UnescapeBatch(Trim(tr.substr(5))));
            } else if (tr.find(L"goto ") == 0) {
            } else {
                backendCommands.push_back(UnescapeBatch(tr));
            }
        }

        if (hasSubmenu) {
            opt.runMode = 2;
            std::wstring subLabel = L"submenu_" + id;
            std::vector<OptionData> subOptions;
            if (ParseGeneratedMenuBlock(lines, subLabel, menuLabel, id, subOptions, depth + 1))
                opt.subOptions = subOptions;
            backendCommands.clear();
        } else {
            opt.commands = backendCommands;
        }
    }

    return true;
}

static bool ParseGeneratedBatchScript(const std::vector<std::wstring>& lines,
    std::wstring& outTitle,
    std::vector<std::wstring>& outDesc,
    std::wstring& outPass,
    std::vector<OptionData>& outOptions,
    bool& outRequireAdmin,
    wchar_t& outBorderChar) {

    if (!IsBatchMenuGeneratorScript(lines)) return false;

    std::wstring line0 = Trim(lines[0]);
    outRequireAdmin = (line0.size() >= 2 && line0.substr(0, 2) == L"::");
    outTitle = L"";
    outDesc.clear();
    outPass.clear();
    outOptions.clear();
    outBorderChar = L'=';

    for (const auto& raw : lines) {
        std::wstring tr = Trim(raw);
        if (tr.size() >= 6 && tr.substr(0, 6) == L"title " && tr != L"title cmd") {
            outTitle = UnescapeBatch(Trim(tr.substr(6)));
            break;
        }
    }

    for (const auto& raw : lines) {
        std::wstring tr = Trim(raw);
        if (tr.find(L"pass_input") != std::wstring::npos && tr.find(L"goto menu") != std::wstring::npos) {
            size_t eqPos = tr.find(L"==");
            if (eqPos != std::wstring::npos) {
                size_t startQ = tr.find(L"\"", eqPos);
                std::wstring parsedPass;
                if (ExtractQuotedBatchValue(tr, startQ, parsedPass))
                    outPass = parsedPass;
            }
            break;
        }
    }

    for (const auto& raw : lines) {
        std::wstring tr = Trim(raw);
        if (tr.size() >= 5 && tr.substr(0, 5) == L"echo ") {
            std::wstring content;
            if (!GetBatchEchoContent(raw, content)) continue;
            content = UnescapeBatch(content);
            if (content.size() >= 20) {
                bool allSame = true;
                wchar_t firstC = content.front();
                for (wchar_t c : content) {
                    if (c != firstC) { allSame = false; break; }
                }
                if (allSame) {
                    outBorderChar = firstC;
                    break;
                }
            }
        }
    }

    size_t menuIdx = std::wstring::npos;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (Trim(lines[i]) == L":menu") {
            menuIdx = i;
            break;
        }
    }

    if (menuIdx != std::wstring::npos) {
        std::vector<size_t> borderIdx;
        for (size_t i = menuIdx + 1; i < lines.size(); ++i) {
            if (Trim(lines[i]).find(L"set /p choice=") == 0) break;
            if (IsEchoBorderLine(lines[i], outBorderChar)) borderIdx.push_back(i);
        }

        if (borderIdx.size() >= 4) {
            std::vector<std::pair<bool, std::wstring>> region;
            for (size_t i = borderIdx[1] + 1; i < borderIdx[2]; ++i) {
                if (Trim(lines[i]) == L"echo.") {
                    region.push_back(std::make_pair(true, std::wstring()));
                    continue;
                }
                std::wstring content;
                if (!GetBatchEchoContent(lines[i], content)) continue;
                region.push_back(std::make_pair(false, UnescapeBatch(content)));
            }
            if (!region.empty() && region.front().first) region.erase(region.begin());
            if (!region.empty() && region.back().first) region.pop_back();
            for (const auto& entry : region) outDesc.push_back(entry.second);
        }
    }

    if (!ParseGeneratedMenuBlock(lines, L"menu", L"close_menu", L"", outOptions, 0))
        return false;

    return true;
}

static bool ParseScriptConfigLines(const std::vector<std::wstring>& lines,
    std::wstring& outTitle,
    std::vector<std::wstring>& outDesc,
    std::wstring& outPass,
    std::vector<OptionData>& outOptions,
    std::wstring& autoFilename,
    std::wstring& autoPath,
    bool& silentPreviewOnly,
    bool& outRequireAdmin,
    wchar_t* outBorderChar = nullptr) {

    outTitle.clear();
    outDesc.clear();
    outPass.clear();
    outOptions.clear();
    autoFilename.clear();
    autoPath.clear();
    silentPreviewOnly = false;
    outRequireAdmin = false;
    if (outBorderChar) *outBorderChar = L'=';

    int optionCount = 0;
    int currentOption = -1;

    std::function<void(size_t&, OptionData&, int)> parseOptionBlock;
    parseOptionBlock = [&](size_t& pos, OptionData& option, int depth) {
        const int MAX_DEPTH = 8;
        while (pos < lines.size()) {
            std::wstring wLine = Trim(lines[pos]);

            if (wLine == L"}") {
                ++pos;
                return;
            }

            if (wLine.empty() || wLine.substr(0, 2) == L"//") {
                ++pos;
                continue;
            }

            if (wLine.substr(0, 8) == L"runmode ") {
                int mode = _wtoi(Trim(wLine.substr(8)).c_str());
                option.runMode = (mode >= 0 && mode <= 2) ? mode : 0;
                ++pos;
                continue;
            }

            if (wLine.size() >= 15 && wLine.substr(0, 15) == L"suboption num: ") {
                int count = _wtoi(Trim(wLine.substr(15)).c_str());
                if (count < 1) count = 1;
                if (count > 20) count = 20;
                option.subOptions.resize((size_t)count);
                ++pos;
                continue;
            }

            if (wLine.size() >= 10 && wLine.substr(0, 10) == L"suboption ") {
                size_t lbl = wLine.find(L" label ");
                size_t brc = wLine.find(L" {");
                if (lbl != std::wstring::npos) {
                    int num = _wtoi(wLine.substr(10, lbl - 10).c_str());
                    if (num >= 1) {
                        if ((size_t)num > option.subOptions.size()) option.subOptions.resize((size_t)std::min<int>(num, 20));
                        if (num <= 20) option.subOptions[(size_t)num - 1].label = Trim(wLine.substr(lbl + 7));
                    }
                    ++pos;
                    continue;
                }
                if (brc != std::wstring::npos) {
                    int num = _wtoi(wLine.substr(10, brc - 10).c_str());
                    if (num >= 1 && num <= 20) {
                        if ((size_t)num > option.subOptions.size()) option.subOptions.resize((size_t)num);
                        ++pos;
                        parseOptionBlock(pos, option.subOptions[(size_t)num - 1], depth + 1);
                        continue;
                    }
                }
            }

            if (depth > 0 && wLine.size() >= 8 && wLine.substr(0, 7) == L"option " && wLine.find(L" {") != std::wstring::npos) {
                size_t brc = wLine.find(L" {");
                int num = _wtoi(wLine.substr(7, brc - 7).c_str());
                if (num >= 1 && num <= 20) {
                    if ((size_t)num > option.subOptions.size()) option.subOptions.resize((size_t)num);
                    ++pos;
                    parseOptionBlock(pos, option.subOptions[(size_t)num - 1], depth + 1);
                    continue;
                }
            }

            option.commands.push_back(wLine);
            ++pos;
        }
    };

    auto resolveOptionPath = [&](const std::vector<size_t>& path) -> OptionData* {
        if (path.empty() || path.size() > 8 || path[0] >= 20) return nullptr;
        if (path[0] >= outOptions.size()) {
            outOptions.resize(path[0] + 1);
            optionCount = (int)outOptions.size();
        }
        OptionData* cur = &outOptions[path[0]];
        for (size_t d = 1; d < path.size(); ++d) {
            if (path[d] >= 20) return nullptr;
            if (path[d] >= cur->subOptions.size()) cur->subOptions.resize(path[d] + 1);
            cur = &cur->subOptions[path[d]];
        }
        return cur;
    };

    auto parseFlatOptionLine = [&](const std::wstring& line, std::vector<size_t>& path, std::wstring& rest) -> bool {
        path.clear();
        rest.clear();
        if (line.size() < 7 || line.compare(0, 7, L"option ") != 0) return false;

        auto readNumber = [&](size_t& at, int& value) -> bool {
            while (at < line.size() && line[at] == L' ') ++at;
            size_t start = at;
            while (at < line.size() && iswdigit(line[at])) ++at;
            if (at == start) return false;
            value = _wtoi(line.substr(start, at - start).c_str());
            return true;
        };

        size_t pos = 7;
        int number = 0;
        if (!readNumber(pos, number) || number < 1) return false;
        path.push_back((size_t)number - 1);

        bool sawSub = false;
        while (true) {
            size_t probe = pos;
            while (probe < line.size() && line[probe] == L' ') ++probe;
            if (line.compare(probe, 10, L"suboption ") != 0) break;
            probe += 10;
            if (!readNumber(probe, number) || number < 1) { path.clear(); return false; }
            path.push_back((size_t)number - 1);
            pos = probe;
            sawSub = true;
        }
        if (!sawSub) { path.clear(); return false; }

        while (pos < line.size() && line[pos] == L' ') ++pos;
        rest = line.substr(pos);
        return true;
    };

    for (size_t i = 0; i < lines.size();) {
        std::wstring wLine = Trim(lines[i]);

        if (wLine.empty() || wLine.substr(0, 2) == L"//") {
            ++i;
            continue;
        }

        if (wLine.size() >= 6 && wLine.substr(0, 6) == L"title ") {
            outTitle = Trim(wLine.substr(6));
            ++i;
            continue;
        }

        if (outBorderChar && wLine.size() >= 7 && wLine.substr(0, 7) == L"border ") {
            std::wstring value = Trim(wLine.substr(7));
            if (!value.empty()) *outBorderChar = value[0];
            ++i;
            continue;
        }

        if (wLine.size() >= 12 && wLine.substr(0, 12) == L"description ") {
            std::wstring d = wLine.size() > 12 ? wLine.substr(12) : L"";
            if (d != L"n" && d != L"no") outDesc.push_back(d);
            ++i;
            continue;
        }

        if ((wLine.size() >= 6 && wLine.substr(0, 6) == L"admin ") ||
            (wLine.size() >= 5 && wLine.substr(0, 5) == L"root ")) {
            std::wstring a = Trim(wLine.substr(wLine.substr(0, 5) == L"root " ? 5 : 6));
            outRequireAdmin = (a == L"y" || a == L"Y" || a == L"yes");
            ++i;
            continue;
        }

        if (wLine.size() >= 5 && wLine.substr(0, 5) == L"pass ") {
            std::wstring p = Trim(wLine.substr(5));
            if (p != L"n" && p != L"no" && !p.empty()) outPass = p;
            ++i;
            continue;
        }

        if (wLine.size() >= 12 && wLine.substr(0, 12) == L"option num: ") {
            optionCount = _wtoi(Trim(wLine.substr(12)).c_str());
            if (optionCount < 1) optionCount = 1;
            if (optionCount > 20) optionCount = 20;
            outOptions.resize((size_t)optionCount);
            ++i;
            continue;
        }

        {
            std::vector<size_t> flatPath;
            std::wstring flatRest;
            if (parseFlatOptionLine(wLine, flatPath, flatRest)) {
                OptionData* target = resolveOptionPath(flatPath);
                if (flatRest.size() >= 6 && flatRest.substr(0, 6) == L"label ") {
                    if (target) target->label = Trim(flatRest.substr(6));
                    ++i;
                } else if (!flatRest.empty() && flatRest[0] == L'{') {
                    ++i;
                    if (target) {
                        parseOptionBlock(i, *target, (int)flatPath.size() - 1);
                    } else {
                        OptionData discarded;
                        parseOptionBlock(i, discarded, 0);
                    }
                } else if (flatRest.size() >= 8 && flatRest.substr(0, 8) == L"runmode ") {
                    if (target) {
                        int mode = _wtoi(Trim(flatRest.substr(8)).c_str());
                        target->runMode = (mode >= 0 && mode <= 2) ? mode : 0;
                    }
                    ++i;
                } else {
                    ++i;
                }
                continue;
            }
        }

        if (wLine.size() >= 7 && wLine.substr(0, 7) == L"option ") {
            size_t lbl = wLine.find(L" label ");
            size_t brc = wLine.find(L" {");

            if (lbl != std::wstring::npos) {
                int num = _wtoi(wLine.substr(7, lbl - 7).c_str());
                if (num >= 1 && num <= optionCount)
                    outOptions[(size_t)num - 1].label = Trim(wLine.substr(lbl + 7));
                ++i;
                continue;
            }

            if (brc != std::wstring::npos) {
                int num = _wtoi(wLine.substr(7, brc - 7).c_str());
                if (num >= 1 && num <= 20) {
                    if (num > optionCount) {
                        optionCount = num;
                        outOptions.resize((size_t)optionCount);
                    }
                    currentOption = num - 1;
                    ++i;
                    parseOptionBlock(i, outOptions[(size_t)currentOption], 0);
                    continue;
                }
            }
        }

        if (wLine.size() >= 8 && wLine.substr(0, 8) == L"runmode ") {
            int mode = _wtoi(Trim(wLine.substr(8)).c_str());
            if (currentOption >= 0 && currentOption < (int)outOptions.size())
                outOptions[(size_t)currentOption].runMode = (mode >= 0 && mode <= 2) ? mode : 0;
            ++i;
            continue;
        }

        if (wLine.size() >= 5 && wLine.substr(0, 5) == L"save ") {
            std::wstring args = Trim(wLine.substr(5));
            if (args == L"n") {
                silentPreviewOnly = true;
            } else {
                size_t pos1 = args.find(L"--");
                if (pos1 != std::wstring::npos) {
                    size_t pos2 = args.find(L"--", pos1 + 2);
                    if (pos2 != std::wstring::npos) {
                        autoFilename = Trim(args.substr(pos1 + 2, pos2 - (pos1 + 2)));
                        autoPath = Trim(args.substr(pos2 + 2));
                    } else {
                        autoFilename = Trim(args.substr(pos1 + 2));
                    }
                }
            }
            ++i;
            continue;
        }

        ++i;
    }

    for (auto& opt : outOptions) {
        if (opt.label.empty()) opt.label = L"Option";
        if (opt.runMode == 2) opt.commands.clear();
        for (auto& sub : opt.subOptions) {
            if (sub.label.empty()) sub.label = L"Option";
            if (sub.runMode == 2) sub.commands.clear();
        }
    }

    return true;
}

static bool ParseScriptConfig(const std::wstring& filePath, std::wstring& outTitle,
    std::vector<std::wstring>& outDesc, std::wstring& outPass, std::vector<OptionData>& outOptions,
    std::wstring& autoFilename, std::wstring& autoPath, bool& silentPreviewOnly,
    bool& outRequireAdmin,
    wchar_t* outBorderChar = nullptr) {
    std::vector<std::wstring> lines;
    if (!ReadConsoleEditorTextFile(filePath, lines)) return false;
    return ParseScriptConfigLines(lines, outTitle, outDesc, outPass, outOptions, autoFilename, autoPath, silentPreviewOnly, outRequireAdmin, outBorderChar);
}

struct OptionTreeRow {
    std::wstring text;
    std::vector<size_t> path;
};

static void BuildOptionTreeRows(const std::vector<OptionData>& options,
    std::vector<OptionTreeRow>& rows,
    std::vector<size_t> path = {}, int depth = 0) {
    for (size_t i = 0; i < options.size(); ++i) {
        std::vector<size_t> currentPath = path;
        currentPath.push_back(i);

        const OptionData& opt = options[i];
        std::wstring label = opt.label.empty() ? (L"Option " + std::to_wstring(i + 1)) : opt.label;
        std::wstring mode = L"RunMode " + std::to_wstring(opt.runMode);
        if (opt.runMode == 2) mode += L" / Sub menu";
        else mode += L", " + std::to_wstring(opt.commands.size()) + L" cmd(s)";

        std::wstring prefix;
        if (depth > 0) prefix = std::wstring((size_t)depth * 4, L' ') + L"\x2514\x2500 ";
        rows.push_back({ prefix + L"Option " + std::to_wstring(i + 1) + L": [" + label + L"] [" + mode + L"]", currentPath });

        if (opt.runMode == 2 && !opt.subOptions.empty())
            BuildOptionTreeRows(opt.subOptions, rows, currentPath, depth + 1);
    }
}

static OptionData* GetOptionByPath(std::vector<OptionData>& roots, const std::vector<size_t>& path) {
    if (path.empty() || path[0] >= roots.size()) return nullptr;
    OptionData* cur = &roots[path[0]];
    for (size_t p = 1; p < path.size(); ++p) {
        if (cur->runMode != 2 || path[p] >= cur->subOptions.size()) return nullptr;
        cur = &cur->subOptions[path[p]];
    }
    return cur;
}

static std::vector<OptionData>* GetOptionContainer(std::vector<OptionData>& roots, const std::vector<size_t>& path) {
    if (path.empty()) return nullptr;
    if (path.size() == 1) return &roots;
    std::vector<size_t> parentPath(path.begin(), path.end() - 1);
    OptionData* parent = GetOptionByPath(roots, parentPath);
    if (!parent || parent->runMode != 2) return nullptr;
    return &parent->subOptions;
}

static OptionData MakeNewOptionInteractive(size_t oneBasedIndex, const wchar_t* heading = L"New Option") {
    OptionData result;
    C_N; wprintf(L"\n  --- %ls ---\n", heading); C_R;
    C_P; wprintf(L"  Label [Option %zu]: ", oneBasedIndex); C_I;
    result.label = ReadLine(); C_R;
    if (result.label.empty()) result.label = L"Option " + std::to_wstring(oneBasedIndex);

    result.runMode = PromptRunMode(0, true);
    if (result.runMode == 2) {
        result.subOptions.clear();
    } else {
        EditLinesWithConsoleEditor(result.commands, true);
    }
    return result;
}

static void ManageOptionsTreeTUI(std::vector<OptionData>& options) {
    while (true) {
        std::vector<OptionTreeRow> rows;
        BuildOptionTreeRows(options, rows);

        std::vector<std::wstring> menu;
        std::vector<std::vector<size_t>> paths;
        for (const auto& row : rows) {
            menu.push_back(row.text);
            paths.push_back(row.path);
        }
        menu.push_back(L"[ Add Root Option ]");
        const int addRootAction = (int)menu.size() - 1;
        menu.push_back(L"[ Back ]");
        const int backAction = (int)menu.size() - 1;

        int choice = RunTUIMenu(L"Manage Options (expanded tree)", menu);
        if (choice == -1 || choice == backAction) return;

        if (choice == addRootAction) {
            if (options.size() >= 20) {
                C_E; wprintf(L"  Maximum 20 root options reached.\n"); C_R; Sleep(800);
                continue;
            }
            options.push_back(MakeNewOptionInteractive(options.size() + 1));
            continue;
        }

        if (choice >= 0 && choice < (int)paths.size()) {
            std::vector<size_t> selectedPath = paths[(size_t)choice];
            OptionData* selected = GetOptionByPath(options, selectedPath);
            if (!selected) continue;

            while (true) {
                std::wstring label = selected->label.empty() ? L"Option" : selected->label;
                std::vector<std::wstring> actions = {
                    L"Edit Label",
                    L"Change RunMode (0/1/2)",
                    L"View/Edit Commands",
                    L"Add Command"
                };
                if (selected->runMode == 2) actions.push_back(L"Add Submenu Option");
                actions.push_back(L"Delete This Option");
                actions.push_back(L"Back");

                int action = RunTUIMenu(L"Edit Option: " + label, actions);
                if (action == -1 || action == (int)actions.size() - 1) break;

                const int addChildAction = (selected->runMode == 2) ? 4 : -1;
                const int deleteAction = (int)actions.size() - 2;

                if (action == 0) {
                    C_P; wprintf(L"  New label [%ls]: ", label.c_str()); C_I;
                    std::wstring nl = ReadLine(); C_R;
                    if (!nl.empty()) selected->label = nl;
                } else if (action == 1) {
                    int oldMode = selected->runMode;
                    int newMode = PromptRunMode(oldMode, true);
                    selected->runMode = newMode;
                    if (newMode == 2) selected->commands.clear();
                    else selected->subOptions.clear();
                } else if (action == 2) {
                    EditCommandsTUI(selected->commands);
                } else if (action == 3) {
                    EditCommandsTUI(selected->commands);
                } else if (action == addChildAction && addChildAction >= 0) {
                    if (selectedPath.size() >= 6) {
                        C_E; wprintf(L"  Maximum submenu depth (6) reached.\n"); C_R; Sleep(800);
                        continue;
                    }
                    if (selected->subOptions.size() >= 20) {
                        C_E; wprintf(L"  Maximum 20 submenu options reached.\n"); C_R; Sleep(800);
                        continue;
                    }
                    selected->subOptions.push_back(MakeNewOptionInteractive(selected->subOptions.size() + 1, L"New Submenu Option"));
                    continue;
                } else if (action == deleteAction) {
                    std::vector<OptionData>* container = GetOptionContainer(options, selectedPath);
                    if (container && selectedPath.back() < container->size()) {
                        container->erase(container->begin() + selectedPath.back());
                        C_S; wprintf(L"  Option deleted from the tree.\n"); C_R; Sleep(700);
                    }
                    break;
                }

                selected = GetOptionByPath(options, selectedPath);
                if (!selected) break;
            }
        }
    }
}

static void ManageOptionsTreeCLI(std::vector<OptionData>& options) {
    while (true) {
        C_H; wprintf(L"\n  --- Manage Options (expanded tree) ---\n"); C_R;
        std::vector<OptionTreeRow> rows;
        BuildOptionTreeRows(options, rows);
        for (size_t i = 0; i < rows.size(); ++i) {
            C_P; wprintf(L"    %zu) %ls\n", i + 1, rows[i].text.c_str()); C_R;
        }
        C_P; wprintf(L"\n    A) Add Root Option\n    E) Edit Option (use tree number)\n    D) Delete Option (use tree number)\n    0) Back\n  Select: "); C_I;
        std::wstring cmd = ReadLine(); C_R;
        if (cmd == L"0") return;

        if (cmd == L"A" || cmd == L"a") {
            if (options.size() >= 20) { C_E; wprintf(L"  Maximum 20 root options reached.\n"); C_R; continue; }
            options.push_back(MakeNewOptionInteractive(options.size() + 1));
            continue;
        }

        bool isEdit = (cmd == L"E" || cmd == L"e");
        bool isDelete = (cmd == L"D" || cmd == L"d");
        if (!isEdit && !isDelete) continue;

        C_P; wprintf(L"  Enter tree number: "); C_I;
        int n = _wtoi(ReadLine().c_str()); C_R;
        if (n < 1 || n > (int)rows.size()) continue;
        const std::vector<size_t> path = rows[(size_t)n - 1].path;
        OptionData* selected = GetOptionByPath(options, path);
        if (!selected) continue;

        if (isDelete) {
            std::vector<OptionData>* container = GetOptionContainer(options, path);
            if (container && path.back() < container->size()) container->erase(container->begin() + path.back());
        } else {
            C_P; wprintf(L"  New Label [%ls]: ", selected->label.c_str()); C_I;
            std::wstring nl = ReadLine(); C_R;
            if (!nl.empty()) selected->label = nl;
            int newMode = PromptRunMode(selected->runMode, true);
            selected->runMode = newMode;
            if (newMode == 2) selected->commands.clear();
            else selected->subOptions.clear();
            if (newMode != 2) {
                EditCommandsTUI(selected->commands);
            }
        }
    }
}

static bool CompileCurrentEditorToExe(
    const std::wstring& title,
    const std::vector<std::wstring>& description,
    const std::wstring& password,
    const std::vector<OptionData>& options,
    bool requireAdmin,
    wchar_t borderChar,
    const std::wstring& loadedFilePath) {

    std::wstring tempPath = GetAppDataDir() +
        L"BMG_editor_compile_" + std::to_wstring(GetCurrentProcessId()) + L".bat.tmp";
    std::wstring script = GenerateBatch(title, description, options, password, requireAdmin, borderChar);

    if (!WriteScriptToFile(tempPath, script, !requireAdmin)) {
        C_E; wprintf(L"  [Error] Could not create temporary batch for EXE compilation.\n"); C_R;
        return false;
    }

    std::wstring suggestedSource = loadedFilePath;
    if (suggestedSource.empty()) {
        wchar_t cwd[MAX_PATH] = {0};
        GetCurrentDirectoryW(MAX_PATH, cwd);
        suggestedSource = std::wstring(cwd) + L"\\BatchMenu.bat";
    }

    std::wstring outputExe;
    bool selected = ChooseExeOutputPath(suggestedSource, outputExe);
    bool ok = selected && CompileBatchToExe(tempPath, outputExe);

    DeleteFileW(tempPath.c_str());
    return ok;
}

static bool RunBatchScriptEditor(std::wstring& title,
    std::vector<std::wstring>& description,
    std::wstring& password,
    std::vector<OptionData>& options,
    bool& requireAdmin,
    wchar_t& borderChar,
    const std::wstring& loadedFilePath) {

    auto saveChangesToScript = [&]() {
        const std::wstring script = GenerateBatch(title, description, options, password, requireAdmin, borderChar);
        if (!WriteScriptToFile(loadedFilePath, script, !requireAdmin)) {
            C_E; wprintf(L"  [Error] Could not save changes to: %ls\n", loadedFilePath.c_str()); C_R;
            return false;
        }
        C_S; wprintf(L"  Changes saved to: %ls\n", loadedFilePath.c_str()); C_R;
        return true;
    };

    while (true) {
        system("cls");
        C_B; TuiBannerBorder();
        C_N; TuiBannerTitle(L"BatchMenuGenerator EDITOR");
        C_B; TuiBannerBorder();
        wprintf(L"\n"); C_T;

        if (g_enableTUI) {
            std::vector<std::wstring> opts;
            opts.push_back(L"Title          : " + (title.empty() ? L"(None)" : title));
            opts.push_back(L"Description    : " + (description.empty() ? L"(None)" : std::to_wstring(description.size()) + L" line(s)"));
            opts.push_back(std::wstring(L"Password       : ") + (password.empty() ? L"(None)" : L"***** [Protected]"));
            opts.push_back(L"Admin Privilege: " + std::wstring(requireAdmin ? L"YES (Enabled)" : L"NO (Disabled)"));
            wchar_t bStr[32]; swprintf(bStr, 32, L"%lc", borderChar);
            opts.push_back(L"Border Style   : " + std::wstring(bStr));
            opts.push_back(L"Manage Options : " + std::to_wstring(options.size()) + L" option(s)");
            opts.push_back(L"Compile to EXE");
            opts.push_back(L"[ Save Changes to Script ]");
            opts.push_back(L"[ Cancel / Return ]");

            int choice = RunTUIMenu(L"Batch Script Editor", opts);
            if (choice == -1 || choice == 8) return false;

            if (choice == 0) {
                std::wstring nt = ReadTitleWithPreview(title);
                if (nt != L"__BMG_CANCEL__") title = nt;
            } else if (choice == 1) {
                EditDescriptionTUI(description);
            } else if (choice == 2) {
                int pChoice = RunTUIMenu(L"Password Options", { L"Set / Change Password", L"Remove Password", L"Back" });
                if (pChoice == 0) {
                    C_P; wprintf(L"\n  Enter new password: "); C_I;
                    password = ReadLine(); C_R;
                    C_S; wprintf(L"  Password updated!\n"); C_R; Sleep(700);
                } else if (pChoice == 1) {
                    password.clear();
                    C_S; wprintf(L"  Password removed!\n"); C_R; Sleep(700);
                }
            } else if (choice == 3) {
                requireAdmin = !requireAdmin;
            } else if (choice == 4) {
                int bIdx = RunTUIMenu(L"Select Border Style", { L"Classic (=)", L"Modern (-)", L"Zigzag (Z)", L"Custom Char" });
                if (bIdx == 0) borderChar = L'=';
                else if (bIdx == 1) borderChar = L'-';
                else if (bIdx == 2) borderChar = L'Z';
                else if (bIdx == 3) {
                    C_P; wprintf(L"  Enter custom border character: "); C_I;
                    std::wstring bc = ReadLine(); C_R;
                    if (!bc.empty()) borderChar = bc[0];
                }
            } else if (choice == 5) {
                ManageOptionsTreeTUI(options);
                        } else if (choice == 6) {
                CompileCurrentEditorToExe(title, description, password, options, requireAdmin, borderChar, loadedFilePath);
                system("pause");
            } else if (choice == 7) {
                if (saveChangesToScript()) return true;
            }
        } else {
            C_P; wprintf(L"    1) Title          : "); C_I;
            wprintf(L"%ls\n", title.empty() ? L"(None)" : title.c_str());
            C_P; wprintf(L"    2) Description    : "); C_I;
            wprintf(L"%ls\n", description.empty() ? L"(None)" : (std::to_wstring(description.size()) + L" line(s)").c_str());
            C_P; wprintf(L"    3) Password       : "); C_I;
            wprintf(L"%ls\n", password.empty() ? L"(None)" : L"***** [Protected]");
            C_P; wprintf(L"    4) Admin Privileges: "); C_I;
            wprintf(L"%ls\n", requireAdmin ? L"YES (Enabled)" : L"NO (Disabled)");
            C_P; wprintf(L"    5) Border Style   : "); C_I; wprintf(L"%lc\n", borderChar);
            C_P; wprintf(L"    6) Manage Options : "); C_I; wprintf(L"%zu option(s)\n", options.size());
            C_P; wprintf(L"    C) Compile to EXE\n");
            C_P; wprintf(L"    S) Save Changes to Script\n    0) Cancel\n\n  Select option: "); C_I;
            std::wstring edChoice = ReadLine(); C_R;

            if (edChoice == L"0") return false;
            if (edChoice == L"S" || edChoice == L"s") {
                if (saveChangesToScript()) return true;
                continue;
            }
            if (edChoice == L"C" || edChoice == L"c") {
                CompileCurrentEditorToExe(title, description, password, options, requireAdmin, borderChar, loadedFilePath);
                system("pause");
                continue;
            }

            if (edChoice == L"1") {
                std::wstring nt = ReadTitleWithPreview(title);
                if (nt != L"__BMG_CANCEL__") title = nt;
            } else if (edChoice == L"2") {
                EditLinesWithConsoleEditor(description);
            } else if (edChoice == L"3") {
                C_P; wprintf(L"  Password Options:\n    1) Set/Change Password\n    2) Remove Password\n  Select [1]: "); C_I;
                std::wstring pc = ReadLine(); C_R;
                if (pc == L"2") password.clear();
                else {
                    C_P; wprintf(L"  Enter password: "); C_I; password = ReadLine(); C_R;
                }
            } else if (edChoice == L"4") {
                requireAdmin = !requireAdmin;
            } else if (edChoice == L"5") {
                C_P; wprintf(L"  Enter border character (=, -, Z, or custom): "); C_I;
                std::wstring bc = ReadLine(); C_R;
                if (!bc.empty()) borderChar = bc[0];
            } else if (edChoice == L"6") {
                ManageOptionsTreeCLI(options);
            }
        }
    }
}

static bool OpenGeneratedBatchEditor(
    std::wstring& title,
    std::vector<std::wstring>& description,
    std::wstring& password,
    std::vector<OptionData>& options,
    bool& requireAdmin,
    wchar_t& borderChar,
    std::wstring& finalSavedPath,
    const std::wstring& requestedPath = L"") {
    std::wstring batPath = requestedPath;
    if (batPath.empty() && g_enableTUI) {
        int lc = RunTUIMenu(L"Choose Batch File to Edit",
            { L"Browse (Windows Explorer)", L"Type path manually", L"Cancel" });
        if (lc == 0) {
            wchar_t path[MAX_PATH] = {0}; OPENFILENAMEW of; ZeroMemory(&of, sizeof(of));
            of.lStructSize = sizeof(of); of.lpstrFilter = L"Batch Files (*.bat)\0*.bat\0All Files (*.*)\0*.*\0\0";
            of.lpstrFile = path; of.nMaxFile = MAX_PATH; of.lpstrDefExt = L"bat";
            of.lpstrTitle = L"Select Generated Batch File"; of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            if (GetOpenFileNameW(&of)) batPath = path;
        } else if (lc == 1) {
            C_P; wprintf(L"\n  Enter batch file path: "); C_I; batPath = ReadLine(); C_R;
        }
    } else if (batPath.empty()) {
        C_P; wprintf(L"\n  Choose Load Method:\n    1 = Browse\n    2 = Type path\n    3 = Cancel\n  Select [1]: "); C_I;
        std::wstring lc = ReadLine(); C_R;
        if (lc == L"1" || lc.empty()) {
            wchar_t path[MAX_PATH] = {0}; OPENFILENAMEW of; ZeroMemory(&of, sizeof(of));
            of.lStructSize = sizeof(of); of.lpstrFilter = L"Batch Files (*.bat)\0*.bat\0All Files (*.*)\0*.*\0\0";
            of.lpstrFile = path; of.nMaxFile = MAX_PATH; of.lpstrDefExt = L"bat";
            of.lpstrTitle = L"Select Generated Batch File"; of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            if (GetOpenFileNameW(&of)) batPath = path;
        } else if (lc == L"2") {
            C_P; wprintf(L"  Enter batch file path: "); C_I; batPath = ReadLine(); C_R;
        }
    }

    if (batPath.empty()) return false;
    if (batPath.length() >= 2 && batPath.front() == L'"' && batPath.back() == L'"')
        batPath = batPath.substr(1, batPath.length() - 2);

    std::vector<std::wstring> bLines;
    if (!ReadConsoleEditorTextFile(batPath, bLines)) {
        C_E; wprintf(L"\n  [Error] Could not open file: %ls\n", batPath.c_str()); C_R;
        return false;
    }

    if (!IsBatchMenuGeneratorScript(bLines)) {
        C_E; wprintf(L"\n  [Error] This script was not created by BatchMenuGenerator!\n");
        wprintf(L"  Missing required watermark on Line 1: 'Created with BatchMenuGenerator!'\n"); C_R;
        return false;
    }

    if (!ParseGeneratedBatchScript(bLines, title, description, password, options, requireAdmin, borderChar)) {
        C_E; wprintf(L"\n  [Error] Failed to parse generated batch file structure.\n"); C_R;
        return false;
    }

    if (!RunBatchScriptEditor(title, description, password, options, requireAdmin, borderChar, batPath))
        return false;

    finalSavedPath = batPath;
    return true;
}

static bool CompileGeneratedScriptToExeInteractive(
    const std::wstring& script,
    const std::wstring& suggestedBatPath) {

    std::wstring tempPath = GetAppDataDir() +
        L"BMG_compile_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
        std::to_wstring(GetTickCount64()) + L".bat.tmp";

    if (!WriteScriptToFile(tempPath, script, true)) {
        C_E; wprintf(L"  [Error] Could not create temporary batch for EXE compilation.\n"); C_R;
        return false;
    }

    std::wstring suggestion = suggestedBatPath;
    if (suggestion.empty()) {
        wchar_t cwd[MAX_PATH] = {0};
        GetCurrentDirectoryW(MAX_PATH, cwd);
        suggestion = std::wstring(cwd) + L"\\BatchMenu.bat";
    }

    std::wstring outputExe;
    bool selected = ChooseExeOutputPath(suggestion, outputExe);
    bool ok = selected && CompileBatchToExe(tempPath, outputExe);

    DeleteFileW(tempPath.c_str());
    return ok;
}

static bool EnsureDirectoryTree(const std::wstring& directory) {
    std::wstring normalized = directory;
    for (auto& ch : normalized) {
        if (ch == L'/') ch = L'\\';
    }
    while (normalized.size() > 3 && normalized.back() == L'\\') normalized.pop_back();
    if (normalized.empty()) return false;
    if (DirectoryExists(normalized)) return true;

    size_t pos = 0;
    while ((pos = normalized.find(L'\\', pos)) != std::wstring::npos) {
        std::wstring partial = normalized.substr(0, pos);
        if (!partial.empty() && partial.back() != L':') CreateDirectoryW(partial.c_str(), NULL);
        ++pos;
    }
    CreateDirectoryW(normalized.c_str(), NULL);
    return DirectoryExists(normalized);
}

static bool SaveConfigScriptAsExe(const std::wstring& script, const std::wstring& fileName,
    const std::wstring& directory, bool requireAdmin) {

    std::wstring dir = Trim(directory);
    if (dir.size() >= 2 && dir.front() == L'"' && dir.back() == L'"')
        dir = Trim(dir.substr(1, dir.size() - 2));
    if (dir.empty()) {
        wchar_t cwd[MAX_PATH] = {0};
        if (GetCurrentDirectoryW(MAX_PATH, cwd)) dir = cwd;
    }
    if (!dir.empty() && !EnsureDirectoryTree(dir)) {
        C_E; wprintf(L"  [Error] Could not create or open the output folder: %ls\n", dir.c_str()); C_R;
        return false;
    }
    if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/') dir += L"\\";
    const std::wstring outputExe = dir + fileName;

    const std::wstring tempPath = GetAppDataDir() +
        L"BMG_save_exe_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
        std::to_wstring(GetTickCount64()) + L".bat.tmp";
    if (!WriteScriptToFile(tempPath, script, !requireAdmin)) {
        C_E; wprintf(L"  [Error] Could not create temporary batch for EXE compilation.\n"); C_R;
        return false;
    }

    const bool ok = CompileBatchToExe(tempPath, outputExe);
    DeleteFileW(tempPath.c_str());
    return ok;
}

struct BmgEditorKey { wchar_t character = 0; WORD virtualKey = 0; DWORD modifiers = 0; };

static WORD ConsoleAttribute(int foreground, int background = CC_BLACK) {
    return (WORD)((foreground & 0x0F) | ((background & 0x0F) << 4));
}

static bool ReadBmgEditorKey(BmgEditorKey& key) {
    INPUT_RECORD event{};
    DWORD read = 0;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    while (ReadConsoleInputW(input, &event, 1, &read)) {
        if (event.EventType != KEY_EVENT || !event.Event.KeyEvent.bKeyDown) continue;
        const KEY_EVENT_RECORD& e = event.Event.KeyEvent;
        key.character = e.uChar.UnicodeChar;
        key.virtualKey = e.wVirtualKeyCode;
        key.modifiers = e.dwControlKeyState;
        return true;
    }
    return false;
}

static void EnsureConsoleBufferRows(HANDLE output, SHORT requiredBottom) {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(output, &info) || requiredBottom < info.dwSize.Y) return;
    COORD size = info.dwSize;
    size.Y = (SHORT)(requiredBottom + 1);
    SetConsoleScreenBufferSize(output, size);
}

static void DrawConsoleBufferRow(HANDLE output, SHORT y, const std::wstring& text,
    WORD attribute, const std::vector<WORD>* perCharacter = nullptr) {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(output, &info) || y < 0 || y >= info.dwSize.Y) return;
    const DWORD width = (DWORD)info.dwSize.X;
    COORD start{0, y};
    DWORD written = 0;
    std::wstring row = text.substr(0, width);
    if (!row.empty()) WriteConsoleOutputCharacterW(output, row.c_str(), (DWORD)row.size(), start, &written);
    if (row.size() < width) FillConsoleOutputCharacterW(output, L' ', width - (DWORD)row.size(), COORD{(SHORT)row.size(), y}, &written);
    if (perCharacter && !perCharacter->empty()) {
        std::vector<WORD> attrs(width, attribute);
        for (size_t i = 0; i < row.size() && i < perCharacter->size(); ++i) attrs[i] = (*perCharacter)[i];
        WriteConsoleOutputAttribute(output, attrs.data(), width, start, &written);
    } else FillConsoleOutputAttribute(output, attribute, width, start, &written);
}

static bool SetClipboardUnicodeText(const std::wstring& value) {
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();
    const SIZE_T bytes = (value.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) { CloseClipboard(); return false; }
    void* data = GlobalLock(memory);
    if (!data) { GlobalFree(memory); CloseClipboard(); return false; }
    memcpy(data, value.c_str(), bytes);
    GlobalUnlock(memory);
    if (!SetClipboardData(CF_UNICODETEXT, memory)) { GlobalFree(memory); CloseClipboard(); return false; }
    CloseClipboard();
    return true;
}

static std::wstring GetClipboardUnicodeText() {
    std::wstring value;
    if (!OpenClipboard(nullptr)) return value;
    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data) {
        const wchar_t* text = (const wchar_t*)GlobalLock(data);
        if (text) { value = text; GlobalUnlock(data); }
    }
    CloseClipboard();
    return value;
}

static bool SaveConsoleEditorTextToPath(const std::vector<std::wstring>& lines, const std::wstring& path) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    file.write((const char*)bom, sizeof(bom));
    for (size_t i = 0; i < lines.size(); ++i) {
        int byteCount = WideCharToMultiByte(CP_UTF8, 0, lines[i].c_str(), (int)lines[i].size(), NULL, 0, NULL, NULL);
        if (byteCount > 0) {
            std::vector<char> utf8((size_t)byteCount);
            if (WideCharToMultiByte(CP_UTF8, 0, lines[i].c_str(), (int)lines[i].size(), utf8.data(), byteCount, NULL, NULL) <= 0)
                return false;
            file.write(utf8.data(), byteCount);
        }
        if (i + 1 < lines.size()) file.write("\r\n", 2);
    }
    return file.good();
}

static bool SaveConsoleEditorTextAs(const std::vector<std::wstring>& lines, std::wstring& savedPath) {
    wchar_t path[MAX_PATH] = L"";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetConsoleWindow();
    dialog.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"txt";
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameW(&dialog)) return false;
    if (!SaveConsoleEditorTextToPath(lines, path)) return false;
    savedPath = path;
    return true;
}

static bool ConsoleTextEditor(std::vector<std::wstring>& lines, const std::wstring& savePath) {
    struct Position { size_t line; size_t col; };
    struct Snapshot { std::vector<std::wstring> lines; size_t line; size_t col; };

    if (lines.empty()) lines.push_back(L"");
    size_t curLine = 0;
    size_t curCol = 0;
    Position selectionAnchor{0, 0};
    bool hasSelection = false;
    std::vector<Snapshot> undoStack;
    std::vector<Snapshot> redoStack;
    std::wstring statusMessage;
    std::wstring lastSearch;
    size_t viewTopLine = 0;
    size_t horizontalOffset = 0;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD originalInputMode = 0;
    const bool haveInputMode = GetConsoleMode(hIn, &originalInputMode) != 0;
    const bool changedInputMode = haveInputMode &&
        SetConsoleMode(hIn, originalInputMode & ~ENABLE_PROCESSED_INPUT) != 0;
    CONSOLE_CURSOR_INFO originalCursorInfo{};
    const bool haveCursorInfo = GetConsoleCursorInfo(hOut, &originalCursorInfo) != 0;
    auto restoreEditorConsole = [&]() {
        if (changedInputMode) SetConsoleMode(hIn, originalInputMode);
        if (haveCursorInfo) SetConsoleCursorInfo(hOut, &originalCursorInfo);
    };
    if (haveCursorInfo) {
        CONSOLE_CURSOR_INFO visible = originalCursorInfo;
        visible.bVisible = TRUE;
        SetConsoleCursorInfo(hOut, &visible);
    }

    struct SelectionRange { Position first; Position last; };
    auto positionLess = [](const Position& a, const Position& b) {
        return a.line < b.line || (a.line == b.line && a.col < b.col);
    };
    auto currentPosition = [&]() -> Position { return Position{curLine, curCol}; };
    auto getSelectionRange = [&]() -> SelectionRange {
        if (positionLess(currentPosition(), selectionAnchor))
            return SelectionRange{currentPosition(), selectionAnchor};
        return SelectionRange{selectionAnchor, currentPosition()};
    };
    auto clearSelection = [&]() {
        hasSelection = false;
        selectionAnchor = currentPosition();
    };
    auto clampCaret = [&]() {
        if (lines.empty()) lines.push_back(L"");
        if (curLine >= lines.size()) curLine = lines.size() - 1;
        if (curCol > lines[curLine].size()) curCol = lines[curLine].size();
        if (selectionAnchor.line >= lines.size()) selectionAnchor.line = lines.size() - 1;
        if (selectionAnchor.col > lines[selectionAnchor.line].size())
            selectionAnchor.col = lines[selectionAnchor.line].size();
        if (selectionAnchor.line == curLine && selectionAnchor.col == curCol) hasSelection = false;
    };
    auto saveUndo = [&]() {
        undoStack.push_back(Snapshot{lines, curLine, curCol});
        if (undoStack.size() > 200) undoStack.erase(undoStack.begin());
        redoStack.clear();
    };
    auto selectedText = [&]() -> std::wstring {
        if (!hasSelection) return L"";
        SelectionRange range = getSelectionRange();
        if (range.first.line == range.last.line)
            return lines[range.first.line].substr(range.first.col, range.last.col - range.first.col);
        std::wstring result = lines[range.first.line].substr(range.first.col);
        result += L"\r\n";
        for (size_t i = range.first.line + 1; i < range.last.line; ++i) {
            result += lines[i];
            result += L"\r\n";
        }
        result += lines[range.last.line].substr(0, range.last.col);
        return result;
    };
    auto eraseSelection = [&]() {
        if (!hasSelection) return;
        SelectionRange range = getSelectionRange();
        if (range.first.line == range.last.line) {
            lines[range.first.line].erase(range.first.col, range.last.col - range.first.col);
        } else {
            std::wstring merged = lines[range.first.line].substr(0, range.first.col) +
                lines[range.last.line].substr(range.last.col);
            lines[range.first.line] = merged;
            lines.erase(lines.begin() + (std::ptrdiff_t)range.first.line + 1,
                lines.begin() + (std::ptrdiff_t)range.last.line + 1);
        }
        curLine = range.first.line;
        curCol = range.first.col;
        clearSelection();
        clampCaret();
    };
    auto replaceSelectionWith = [&](const std::wstring& replacement) {
        if (hasSelection) eraseSelection();
        std::wstring prefix = lines[curLine].substr(0, curCol);
        std::wstring suffix = lines[curLine].substr(curCol);
        std::vector<std::wstring> parts;
        std::wstring part;
        for (size_t i = 0; i < replacement.size(); ++i) {
            wchar_t ch = replacement[i];
            if (ch == L'\r') {
                if (i + 1 < replacement.size() && replacement[i + 1] == L'\n') ++i;
                parts.push_back(part);
                part.clear();
            } else if (ch == L'\n') {
                parts.push_back(part);
                part.clear();
            } else {
                part.push_back(ch);
            }
        }
        parts.push_back(part);
        if (parts.size() == 1) {
            lines[curLine] = prefix + parts[0] + suffix;
            curCol = prefix.size() + parts[0].size();
        } else {
            lines[curLine] = prefix + parts.front();
            std::vector<std::wstring>::iterator insertAt = lines.begin() + (std::ptrdiff_t)curLine + 1;
            for (size_t i = 1; i + 1 < parts.size(); ++i)
                insertAt = lines.insert(insertAt, parts[i]) + 1;
            lines.insert(insertAt, parts.back() + suffix);
            curLine += parts.size() - 1;
            curCol = parts.back().size();
        }
        clearSelection();
        clampCaret();
    };
    auto beginMove = [&](bool shift) {
        if (shift) {
            if (!hasSelection) selectionAnchor = currentPosition();
        } else {
            clearSelection();
        }
    };
    auto endMove = [&](bool shift) {
        hasSelection = shift && !(selectionAnchor.line == curLine && selectionAnchor.col == curCol);
    };
    auto moveWordLeft = [&]() {
        if (curCol == 0) {
            if (curLine > 0) { --curLine; curCol = lines[curLine].size(); }
            return;
        }
        const std::wstring& line = lines[curLine];
        while (curCol > 0 && std::iswspace(line[curCol - 1])) --curCol;
        while (curCol > 0 && !std::iswspace(line[curCol - 1])) --curCol;
    };
    auto moveWordRight = [&]() {
        const std::wstring& line = lines[curLine];
        if (curCol >= line.size()) {
            if (curLine + 1 < lines.size()) { ++curLine; curCol = 0; }
            return;
        }
        while (curCol < line.size() && !std::iswspace(line[curCol])) ++curCol;
        while (curCol < line.size() && std::iswspace(line[curCol])) ++curCol;
    };
    auto erasePreviousWord = [&]() {
        if (hasSelection) { eraseSelection(); return; }
        if (curCol == 0) {
            if (curLine == 0) return;
            size_t previousLength = lines[curLine - 1].size();
            lines[curLine - 1] += lines[curLine];
            lines.erase(lines.begin() + (std::ptrdiff_t)curLine);
            --curLine;
            curCol = previousLength;
        }
        size_t begin = curCol;
        while (begin > 0 && std::iswspace(lines[curLine][begin - 1])) --begin;
        while (begin > 0 && !std::iswspace(lines[curLine][begin - 1])) --begin;
        lines[curLine].erase(begin, curCol - begin);
        curCol = begin;
    };
    auto eraseNextWord = [&]() {
        if (hasSelection) { eraseSelection(); return; }
        if (curCol >= lines[curLine].size()) {
            if (curLine + 1 >= lines.size()) return;
            lines[curLine] += lines[curLine + 1];
            lines.erase(lines.begin() + (std::ptrdiff_t)curLine + 1);
        }
        size_t finish = curCol;
        while (finish < lines[curLine].size() && std::iswspace(lines[curLine][finish])) ++finish;
        while (finish < lines[curLine].size() && !std::iswspace(lines[curLine][finish])) ++finish;
        lines[curLine].erase(curCol, finish - curCol);
    };

    auto redraw = [&]() {
        clampCaret();
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
        const int width = (int)csbi.dwSize.X;
        const int windowTop = (int)csbi.srWindow.Top;
        const int windowBottom = (int)csbi.srWindow.Bottom;
        const int visibleHeight = windowBottom - windowTop + 1;
        const int textStartY = windowTop + 3;
        const int textRows = std::max(1, visibleHeight - 5);
        EnsureConsoleBufferRows(hOut, windowBottom + 2);

        if (curLine < viewTopLine) viewTopLine = curLine;
        else if (curLine >= viewTopLine + (size_t)textRows)
            viewTopLine = curLine - (size_t)textRows + 1;
        if (lines.size() <= (size_t)textRows) viewTopLine = 0;
        else if (viewTopLine > lines.size() - (size_t)textRows)
            viewTopLine = lines.size() - (size_t)textRows;

        const size_t editWidth = (size_t)std::max(1, width - 3);
        if (curCol < horizontalOffset) horizontalOffset = curCol;
        else if (curCol >= horizontalOffset + editWidth)
            horizontalOffset = curCol - editWidth + 1;

        const WORD normalAttr = (WORD)g_theme.text;
        DrawConsoleBufferRow(hOut, windowTop,
            L" BMG CLI Editor | Ctrl+S " + std::wstring(savePath.empty() ? L"Save As" : L"Save") +
                L" | Ctrl+Z Undo | Ctrl+Y Redo | Esc Menu",
            (WORD)g_theme.header);
        DrawConsoleBufferRow(hOut, windowTop + 1,
            L" Arrows/Home/End | Ctrl+Arrows: words | Ctrl+A/X/V | Ctrl+Insert: copy | Enter: split line",
            (WORD)g_theme.prompt);
        DrawConsoleBufferRow(hOut, windowTop + 2, L"", normalAttr);

        const WORD selectionAttr = ConsoleAttribute(CC_BLACK, g_theme.accent);
        for (int row = 0; row < textRows; ++row) {
            size_t lineIndex = viewTopLine + (size_t)row;
            const int screenY = textStartY + row;
            if (lineIndex >= lines.size()) {
                DrawConsoleBufferRow(hOut, screenY, L"", normalAttr);
                continue;
            }

            std::wstring prefix = (lineIndex == curLine) ? L"> " : L"  ";
            std::wstring visibleText;
            if (horizontalOffset < lines[lineIndex].size())
                visibleText = lines[lineIndex].substr(horizontalOffset, editWidth);
            std::wstring display = prefix + visibleText;
            std::vector<WORD> attrs(display.size(), normalAttr);
            attrs[0] = (WORD)g_theme.accent;
            attrs[1] = (WORD)g_theme.accent;

            if (hasSelection) {
                SelectionRange range = getSelectionRange();
                if (lineIndex >= range.first.line && lineIndex <= range.last.line) {
                    size_t selStart = (lineIndex == range.first.line) ? range.first.col : 0;
                    size_t selEnd = (lineIndex == range.last.line) ? range.last.col : lines[lineIndex].size();
                    selStart = std::max(selStart, horizontalOffset);
                    selEnd = std::min(selEnd, horizontalOffset + editWidth);
                    for (size_t col = selStart; col < selEnd; ++col) {
                        size_t screenX = 2 + col - horizontalOffset;
                        if (screenX < attrs.size()) attrs[screenX] = selectionAttr;
                    }
                    if (lineIndex < range.last.line && attrs.size() < (size_t)width)
                        attrs.resize(std::min((size_t)width, display.size() + 1), selectionAttr);
                }
            }
            DrawConsoleBufferRow(hOut, screenY, display, normalAttr, &attrs);
        }

        std::wstring help = L" Ctrl+F/F3 Find | Shift+Arrows Select | Ctrl+Home/End Document | Ctrl+Insert Copy | Shift+Insert Paste";
        DrawConsoleBufferRow(hOut, windowBottom - 1, help, (WORD)g_theme.info);
        std::wstring status = statusMessage.empty() ? L" Ready" : L" " + statusMessage;
        status += L" | Ln " + std::to_wstring(curLine + 1) + L", Col " + std::to_wstring(curCol + 1) +
            L" | " + std::to_wstring(lines.size()) + L" line(s)";
        if (hasSelection) status += L" | Selection";
        DrawConsoleBufferRow(hOut, windowBottom, status, (WORD)g_theme.prompt);

        const int caretY = textStartY + (int)(curLine - viewTopLine);
        const int caretX = std::min(width - 1, 2 + (int)curCol - (int)horizontalOffset);
        COORD caret = { (SHORT)std::max(0, caretX), (SHORT)std::max(windowTop, caretY) };
        SetConsoleCursorPosition(hOut, caret);
        fflush(stdout);
    };

    auto readPromptText = [&](const std::wstring& prompt) -> std::wstring {
        std::wstring value;
        while (true) {
            CONSOLE_SCREEN_BUFFER_INFO csbi{};
            if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return L"";
            DrawConsoleBufferRow(hOut, csbi.srWindow.Bottom,
                prompt + value + L"_ (Enter=confirm, Esc=cancel)", (WORD)g_theme.prompt);
            BmgEditorKey key{};
            if (!ReadBmgEditorKey(key)) return L"";
            if (key.virtualKey == VK_ESCAPE) return L"";
            if (key.virtualKey == VK_RETURN) return value;
            if (key.virtualKey == VK_BACK) {
                if (!value.empty()) value.pop_back();
            } else if (key.character >= 32 && key.character != 127) {
                value.push_back(key.character);
            }
        }
    };
    auto findNext = [&](const std::wstring& query) -> bool {
        if (query.empty()) return false;
        auto lower = [](wchar_t ch) { return (wchar_t)std::towlower(ch); };
        Position start{curLine, curCol};
        if (hasSelection) start = getSelectionRange().last;
        auto checkLine = [&](size_t lineIndex, size_t beginAt, size_t endBefore) -> bool {
            const std::wstring& line = lines[lineIndex];
            if (line.size() < query.size()) return false;
            const size_t lastStart = std::min(line.size() - query.size(), endBefore);
            if (beginAt > lastStart) return false;
            for (size_t col = beginAt; col <= lastStart; ++col) {
                bool match = true;
                for (size_t j = 0; j < query.size(); ++j) {
                    if (lower(line[col + j]) != lower(query[j])) { match = false; break; }
                }
                if (match) {
                    selectionAnchor = Position{lineIndex, col};
                    curLine = lineIndex;
                    curCol = col + query.size();
                    hasSelection = true;
                    statusMessage = L"Found: " + query;
                    return true;
                }
            }
            return false;
        };
        for (size_t pass = 0; pass < lines.size(); ++pass) {
            size_t lineIndex = (start.line + pass) % lines.size();
            size_t beginAt = (pass == 0) ? start.col : 0;
            if (checkLine(lineIndex, beginAt, lines[lineIndex].size())) return true;
        }
        if (start.col > 0 && checkLine(start.line, 0, std::min(start.col - 1, lines[start.line].size())))
            return true;
        statusMessage = L"Not found: " + query;
        return false;
    };

    system("cls");
    redraw();
    while (true) {
        BmgEditorKey key{};
        if (!ReadBmgEditorKey(key)) {
            restoreEditorConsole();
            return false;
        }
        const bool ctrl = (key.modifiers & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
        const bool shift = (key.modifiers & SHIFT_PRESSED) != 0;
        statusMessage.clear();
        bool changedText = false;
        bool moved = false;

        if (key.virtualKey == VK_ESCAPE) {
            int opt = -1;
            if (g_enableTUI) {
                opt = RunTUIMenu(L"ESC Menu", { L"Continue", L"Compile (finish)", L"Exit to Main Menu", L"Exit without Save" });
            } else {
                CONSOLE_SCREEN_BUFFER_INFO csbi{};
                GetConsoleScreenBufferInfo(hOut, &csbi);
                DrawConsoleBufferRow(hOut, csbi.srWindow.Bottom,
                    L" ESC Menu: 1 Continue | 2 Compile/Finish | 3 Main Menu | 4 Exit Program",
                    (WORD)g_theme.prompt);
                while (true) {
                    BmgEditorKey menuKey{};
                    if (!ReadBmgEditorKey(menuKey)) break;
                    if (menuKey.character == L'1') { opt = 0; break; }
                    if (menuKey.character == L'2') { opt = 1; break; }
                    if (menuKey.character == L'3') { opt = 2; break; }
                    if (menuKey.character == L'4') { opt = 3; break; }
                    if (menuKey.virtualKey == VK_ESCAPE) { opt = 0; break; }
                }
            }
            if (opt == 0 || opt == -1) {
                statusMessage = L"Editing";
                system("cls");
                redraw();
                continue;
            }
            restoreEditorConsole();
            system("cls");
            if (opt == 1) return true;
            if (opt == 2) return false;
            if (opt == 3) exit(0);
            continue;
        }

        if (ctrl && (key.virtualKey == 'A' || key.character == 1)) {
            selectionAnchor = Position{0, 0};
            curLine = lines.size() - 1;
            curCol = lines.back().size();
            hasSelection = true;
            moved = true;
        } else if (ctrl && key.virtualKey == VK_INSERT) {
            std::wstring value = hasSelection ? selectedText() : lines[curLine];
            if (SetClipboardUnicodeText(value)) statusMessage = L"Copied";
            else statusMessage = L"Clipboard copy failed";
        } else if ((ctrl && (key.virtualKey == 'X' || key.character == 24)) ||
                   (shift && key.virtualKey == VK_DELETE)) {
            if (!hasSelection) {
                selectionAnchor = Position{curLine, 0};
                curCol = lines[curLine].size();
                hasSelection = true;
            }
            std::wstring value = selectedText();
            if (SetClipboardUnicodeText(value)) {
                saveUndo();
                eraseSelection();
                statusMessage = L"Cut";
                changedText = true;
            } else statusMessage = L"Clipboard cut failed";
        } else if ((ctrl && (key.virtualKey == 'V' || key.character == 22)) ||
                   (shift && key.virtualKey == VK_INSERT)) {
            std::wstring paste = GetClipboardUnicodeText();
            if (!paste.empty()) {
                saveUndo();
                replaceSelectionWith(paste);
                statusMessage = L"Pasted";
                changedText = true;
            }
        } else if (ctrl && (key.virtualKey == 'Z' || key.character == 26)) {
            if (!undoStack.empty()) {
                redoStack.push_back(Snapshot{lines, curLine, curCol});
                Snapshot state = undoStack.back(); undoStack.pop_back();
                lines = state.lines; curLine = state.line; curCol = state.col;
                clearSelection(); clampCaret(); statusMessage = L"Undo"; moved = true;
            } else statusMessage = L"Nothing to undo";
        } else if (ctrl && (key.virtualKey == 'Y' || key.character == 25)) {
            if (!redoStack.empty()) {
                undoStack.push_back(Snapshot{lines, curLine, curCol});
                Snapshot state = redoStack.back(); redoStack.pop_back();
                lines = state.lines; curLine = state.line; curCol = state.col;
                clearSelection(); clampCaret(); statusMessage = L"Redo"; moved = true;
            } else statusMessage = L"Nothing to redo";
        } else if ((ctrl && key.virtualKey == 'S') || (ctrl && key.character == 19)) {
            std::wstring savedPath;
            if (!savePath.empty()) {
                if (SaveConsoleEditorTextToPath(lines, savePath)) statusMessage = L"Saved: " + savePath;
                else statusMessage = L"Save failed: " + savePath;
            } else if (SaveConsoleEditorTextAs(lines, savedPath)) statusMessage = L"Saved: " + savedPath;
            else statusMessage = L"Save cancelled or failed";
        } else if ((ctrl && key.virtualKey == 'F') || (ctrl && key.character == 6)) {
            std::wstring query = readPromptText(L" Find: ");
            moved = true;
            if (!query.empty()) { lastSearch = query; findNext(lastSearch); }
        } else if (key.virtualKey == VK_F3) {
            if (lastSearch.empty()) statusMessage = L"Use Ctrl+F to enter a search";
            else { findNext(lastSearch); moved = true; }
        } else if (ctrl && (key.virtualKey == VK_HOME || key.virtualKey == VK_END)) {
            beginMove(shift);
            if (key.virtualKey == VK_HOME) { curLine = 0; curCol = 0; }
            else { curLine = lines.size() - 1; curCol = lines.back().size(); }
            endMove(shift); moved = true;
        } else if (ctrl && (key.virtualKey == VK_LEFT || key.virtualKey == VK_RIGHT)) {
            beginMove(shift);
            if (key.virtualKey == VK_LEFT) moveWordLeft(); else moveWordRight();
            endMove(shift); moved = true;
        } else if (key.virtualKey == VK_LEFT) {
            beginMove(shift);
            if (curCol > 0) --curCol;
            else if (curLine > 0) { --curLine; curCol = lines[curLine].size(); }
            endMove(shift); moved = true;
        } else if (key.virtualKey == VK_RIGHT) {
            beginMove(shift);
            if (curCol < lines[curLine].size()) ++curCol;
            else if (curLine + 1 < lines.size()) { ++curLine; curCol = 0; }
            endMove(shift); moved = true;
        } else if (key.virtualKey == VK_UP) {
            beginMove(shift);
            if (curLine > 0) { --curLine; curCol = std::min(curCol, lines[curLine].size()); }
            endMove(shift); moved = true;
        } else if (key.virtualKey == VK_DOWN) {
            beginMove(shift);
            if (curLine + 1 < lines.size()) { ++curLine; curCol = std::min(curCol, lines[curLine].size()); }
            endMove(shift); moved = true;
        } else if (key.virtualKey == VK_HOME) {
            beginMove(shift); curCol = 0; endMove(shift); moved = true;
        } else if (key.virtualKey == VK_END) {
            beginMove(shift); curCol = lines[curLine].size(); endMove(shift); moved = true;
        } else if (key.virtualKey == VK_RETURN) {
            saveUndo();
            std::wstring tail = lines[curLine].substr(curCol);
            lines[curLine].erase(curCol);
            lines.insert(lines.begin() + (std::ptrdiff_t)curLine + 1, tail);
            ++curLine; curCol = 0; clearSelection(); changedText = true;
        } else if (key.virtualKey == VK_BACK) {
            if (ctrl) {
                size_t oldLine = curLine, oldCol = curCol;
                std::vector<std::wstring> before = lines;
                if (hasSelection || curCol > 0 || curLine > 0) {
                    saveUndo();
                    erasePreviousWord();
                    changedText = (before != lines || oldLine != curLine || oldCol != curCol);
                }
            } else if (hasSelection) {
                saveUndo(); eraseSelection(); changedText = true;
            } else if (curCol > 0) {
                saveUndo(); lines[curLine].erase(curCol - 1, 1); --curCol; changedText = true;
            } else if (curLine > 0) {
                saveUndo(); size_t oldLength = lines[curLine - 1].size();
                lines[curLine - 1] += lines[curLine];
                lines.erase(lines.begin() + (std::ptrdiff_t)curLine);
                --curLine; curCol = oldLength; changedText = true;
            }
            clearSelection();
        } else if (key.virtualKey == VK_DELETE) {
            if (ctrl) {
                std::vector<std::wstring> before = lines;
                if (hasSelection || curCol < lines[curLine].size() || curLine + 1 < lines.size()) {
                    saveUndo();
                    eraseNextWord();
                    changedText = (before != lines);
                }
            } else if (hasSelection) {
                saveUndo(); eraseSelection(); changedText = true;
            } else if (curCol < lines[curLine].size()) {
                saveUndo(); lines[curLine].erase(curCol, 1); changedText = true;
            } else if (curLine + 1 < lines.size()) {
                saveUndo(); lines[curLine] += lines[curLine + 1];
                lines.erase(lines.begin() + (std::ptrdiff_t)curLine + 1); changedText = true;
            }
            clearSelection();
        } else if (key.virtualKey == VK_INSERT && ctrl) {
            std::wstring value = hasSelection ? selectedText() : lines[curLine];
            if (SetClipboardUnicodeText(value)) statusMessage = L"Copied";
        } else if (key.character == L'\t') {
            saveUndo(); replaceSelectionWith(L"    "); changedText = true;
        } else if (key.character >= 32 && key.character != 127 && !ctrl &&
                   (key.modifiers & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) == 0) {
            saveUndo(); replaceSelectionWith(std::wstring(1, key.character)); changedText = true;
        }

        if (changedText) {
            if (lines.empty()) lines.push_back(L"");
            if (statusMessage.empty()) statusMessage = L"Modified";
        }
        if (moved || changedText || !statusMessage.empty()) redraw();
    }
    return false;
}

static void EditLinesWithConsoleEditor(std::vector<std::wstring>& lines, bool removeEmptyLines) {
    std::vector<std::wstring> edited = lines;
    const bool startedEmpty = lines.empty();
    if (!ConsoleTextEditor(edited)) return;

    if (removeEmptyLines) {
        edited.erase(std::remove_if(edited.begin(), edited.end(),
            [](const std::wstring& line) { return line.empty(); }), edited.end());
    } else if (startedEmpty && edited.size() == 1 && edited[0].empty()) {
        edited.clear();
    }
    lines.swap(edited);
}

static bool ReadConsoleEditorTextFile(const std::wstring& path, std::vector<std::wstring>& lines) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::wstring content;
    if (bytes.size() >= 2 && (unsigned char)bytes[0] == 0xFF && (unsigned char)bytes[1] == 0xFE) {
        if ((bytes.size() - 2) % sizeof(wchar_t) != 0) return false;
        content.resize((bytes.size() - 2) / sizeof(wchar_t));
        if (!content.empty()) memcpy(&content[0], bytes.data() + 2, content.size() * sizeof(wchar_t));
    } else {
        size_t offset = (bytes.size() >= 3 && (unsigned char)bytes[0] == 0xEF &&
            (unsigned char)bytes[1] == 0xBB && (unsigned char)bytes[2] == 0xBF) ? 3 : 0;
        const char* data = bytes.data() + offset;
        const int byteCount = (int)(bytes.size() - offset);
        if (byteCount > 0) {
            int charCount = MultiByteToWideChar(CP_UTF8, 0, data, byteCount, NULL, 0);
            if (charCount <= 0) return false;
            content.resize((size_t)charCount);
            if (MultiByteToWideChar(CP_UTF8, 0, data, byteCount, &content[0], charCount) <= 0) return false;
        }
    }

    lines.clear();
    if (content.empty()) return true;
    size_t start = 0;
    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] != L'\r' && content[i] != L'\n') continue;
        lines.push_back(content.substr(start, i - start));
        if (content[i] == L'\r' && i + 1 < content.size() && content[i + 1] == L'\n') ++i;
        start = i + 1;
    }
    if (start < content.size()) lines.push_back(content.substr(start));
    else if (!content.empty() && (content.back() == L'\r' || content.back() == L'\n')) lines.push_back(L"");
    return true;
}


static void OpenInNotepadAndWait(const std::wstring& path) {
    std::wstring cmd = L"notepad.exe \"" + path + L"\"";
    STARTUPINFOW si; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    PROCESS_INFORMATION pi; ZeroMemory(&pi, sizeof(pi));
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(L'\0');
    if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

static bool g_debugMode = false;

static void RenderVersionLine(const std::wstring& line) {
    int baseColor = g_theme.text;
    std::wstring workLine = line;

    if (workLine.rfind(L"[cyan] ", 0) == 0 || workLine.rfind(L"[c] ", 0) == 0) {
        baseColor = g_theme.header;
        workLine = workLine.substr(workLine.find(L' ') + 1);
    } else if (workLine.rfind(L"[yellow] ", 0) == 0 || workLine.rfind(L"[y] ", 0) == 0) {
        baseColor = g_theme.prompt;
        workLine = workLine.substr(workLine.find(L' ') + 1);
    } else if (workLine.rfind(L"[green] ", 0) == 0 || workLine.rfind(L"[g] ", 0) == 0) {
        baseColor = g_theme.success;
        workLine = workLine.substr(workLine.find(L' ') + 1);
    } else if (workLine.rfind(L"[red] ", 0) == 0 || workLine.rfind(L"[r] ", 0) == 0) {
        baseColor = g_theme.error;
        workLine = workLine.substr(workLine.find(L' ') + 1);
    } else if (workLine.rfind(L"[magenta] ", 0) == 0 || workLine.rfind(L"[m] ", 0) == 0) {
        baseColor = g_theme.info;
        workLine = workLine.substr(workLine.find(L' ') + 1);
    } else if (workLine.rfind(L"[white] ", 0) == 0 || workLine.rfind(L"[w] ", 0) == 0) {
        baseColor = g_theme.text;
        workLine = workLine.substr(workLine.find(L' ') + 1);
    }

    SetColor(baseColor);

    size_t i = 0;
    int currentColor = baseColor;

    while (i < workLine.length()) {
        if (workLine[i] == L'{') {
            size_t closeBrace = workLine.find(L'}', i);
            if (closeBrace != std::wstring::npos) {
                std::wstring tag = workLine.substr(i + 1, closeBrace - i - 1);
                std::transform(tag.begin(), tag.end(), tag.begin(), ::towlower);

                bool isTag = false;
                int newColor = currentColor;

                if (tag == L"cyan" || tag == L"c" || tag == L"header") { newColor = g_theme.header; isTag = true; }
                else if (tag == L"yellow" || tag == L"y" || tag == L"prompt") { newColor = g_theme.prompt; isTag = true; }
                else if (tag == L"green" || tag == L"g" || tag == L"success") { newColor = g_theme.success; isTag = true; }
                else if (tag == L"red" || tag == L"r" || tag == L"error") { newColor = g_theme.error; isTag = true; }
                else if (tag == L"white" || tag == L"w" || tag == L"text") { newColor = g_theme.text; isTag = true; }
                else if (tag == L"magenta" || tag == L"m" || tag == L"info") { newColor = g_theme.info; isTag = true; }
                else if (tag == L"/" || tag == L"reset") { newColor = baseColor; isTag = true; }

                if (isTag) {
                    currentColor = newColor;
                    SetColor(currentColor);
                    i = closeBrace + 1;
                    continue;
                }
            }
        }
        wprintf(L"%c", workLine[i]);
        i++;
    }
    wprintf(L"\n");
    C_R;
}

static const wchar_t* const kBmgVersionLabel = L"v1.5.0 Release-Stable";

static const wchar_t* const kPatchNotes[] = {
LR"BMG_NOTES([c] ===================================================================
[c]   BatchMenuGenerator 1.5.0  -  Release-Stable
[c]   Everything that changed since 1.4.3
[c] ===================================================================

1.5.0 is a big release. BatchMenuGenerator used to be a question-and-answer wizard; it is now a small workbench with arrow-key menus, nested sub menus, remembered settings, the ability to re-open and edit scripts it generated earlier, a built-in text editor and a built-in EXE compiler. Scripts and config files made with 1.4.x keep working.

[y] ======================== MAJOR FEATURES ========================

[g] 1. SUB MENUS (NESTED MENUS)
Any option can now open a menu of its own. Choose Run Mode 2 ("Sub menu") for an option and you define the options that appear inside it: up to 20 per menu, nested up to 6 levels deep.
  - Every sub menu shows its own heading and an {y}X) Back{/} entry that returns to the parent menu. The top-level menu keeps {y}X) Exit{/}.
  - Sub menus work everywhere: the wizard, text configs, AutoSave, the option tree editor and the script editor.
  - Run Mode 0 (direct call) and Run Mode 1 (cmd /c wrapper) work exactly as they did in 1.4.3.

[g] 2. ARROW-KEY INTERFACE (TUI)
Menus are driven with {y}Up/Down{/}, {y}Enter{/} and {y}Esc{/} (back). The highlighted row uses the theme's accent color, the selection wraps from the last entry to the first, and the cursor is hidden while a menu is open. This replaces most typed Y/N and number prompts: description, administrator privileges, run mode, border style, save, save method, compile, what-to-do-next, the save-conflict dialogs, Text Mode, Settings, Set Up Program and the editors.
  - Tall menus (20 options plus sub-menu rows) are handled: the console buffer grows automatically so every row can be reached.
  - Prefer typing? Switch {y}TUI Mode{/} off in Settings and the classic prompts come back.
  - Optional menu sounds: a short beep when you move and a higher one when you confirm. Off by default (Settings > Menu Sounds).

[g] 3. SETTINGS THAT ARE REMEMBERED
Main menu > {y}Settings (bconfig){/}. Select an entry to toggle it or cycle through its values. {y}[ Save & Exit ]{/} writes everything to disk; {y}[ Cancel ]{/} or Esc puts everything back, including live theme previews.
  - {y}Theme Engine{/} and {y}Theme Preset{/}: recolors the whole program (this screen too). Presets: default, matrix, cyberpunk, cli, violent. The preset entry appears once the engine is on.
  - {y}AutoSave{/}: on/off for resume-where-you-left-off.
  - {y}TUI Mode{/} and {y}Menu Sounds{/}.
  - {y}End Action{/}: what happens after a script is saved - ask every time, go to the main menu, launch the script, or exit.
  - {y}Saveload{/}: how paths are chosen - ask every time, always the Explorer dialog, or always typed.
  - {y}Menuborder{/}: skip the border question by always using Classic (=), Modern (-) or Zigzag (Z).
  - {y}View Version History{/} (this screen) and {y}Set Up Program{/} are reachable from here too.
Everything is stored in {y}%LOCALAPPDATA%\BatchMenuGenerator\settings.txt{/} as plain {y}key: value{/} lines (lines starting with // are comments), so it is safe to edit by hand. Invalid values are reported as warnings on the main screen, and external edits are picked up as soon as you leave the Settings menu. A settings.txt or bconfig.txt found next to the program from earlier builds is moved into AppData automatically.
)BMG_NOTES",
LR"BMG_NOTES([g] 4. COMPILE TO EXE
Turn a generated batch menu into a stand-alone {y}.exe{/}. You can compile:
  - at the end of the wizard or Text Mode ("Compile this batch into an EXE?"),
  - from the Batch Script Editor ({y}Compile to EXE{/}),
  - from a text config with {y}save --name.exe--C:\folder{/},
  - from the command line with {y}bmg -compile "script.bat"{/} (any .bat file works, not only generated ones).
How it works: the program writes a small C# launcher with your script embedded and builds it with the .NET Framework compiler ({y}csc.exe{/}, found on PATH or under %WINDIR%\Microsoft.NET). If csc.exe is missing it falls back to Windows PowerShell 5.1. Nothing needs to be installed. When the EXE runs it unpacks the script to a temporary .cmd, runs it with cmd, passes its arguments through, returns the script's exit code and deletes the temporary file.
[m] Note: the embedded script is scrambled with a random key per build, so it cannot be read at a glance - but that is obfuscation, not encryption. Do not rely on it to protect secrets.

[g] 5. EDIT SCRIPTS YOU ALREADY MADE
Main menu > {y}Edit existing generated .bat script{/}. Pick a file with the Explorer dialog or type its path (or start the program with {y}bmg -edit "script.bat"{/}). BatchMenuGenerator reads the script back into editable form - title, description, password, administrator flag, border, every option, sub menu and run mode - and opens the Batch Script Editor:
  - {y}Title{/} with a live preview, {y}Description{/} in the full-screen editor, {y}Password{/} (set, change or remove), {y}Admin Privilege{/} on/off and {y}Border Style{/} (Classic, Modern, Zigzag or any character).
  - {y}Manage Options{/}: an expanded tree of all options and sub menus. Add a root option, or open any entry to edit its label, change its run mode, view/edit or add commands, add a sub-menu option (run mode 2) or delete it.
  - {y}Compile to EXE{/}, {y}Save Changes to Script{/} (writes back to the same file) or {y}Cancel / Return{/}.
Only scripts that carry the line-1 watermark {y}Created with BatchMenuGenerator!{/} are accepted. Scripts generated by 1.4.x open too, including their cmd /c options.

[g] 6. TEXT MODE (REPLACES "LOAD TXT CONFIG")
Main menu > {y}Text Mode{/} builds a whole menu from a plain-text description instead of answering questions:
  - {y}Edit in Notepad{/}: opens a scratch config file (kept in the AppData folder, so your last text is still there next time) and builds the menu when Notepad closes.
  - {y}Edit in CLI{/}: the built-in editor (feature 7).
  - {y}Load TXT config{/}: open a config from the Explorer dialog or by typing its path. Files may be UTF-8 (with or without a BOM) or UTF-16 LE.
  - {y}Show syntax tutorial{/}: a built-in reference for every config directive, with examples.
Configs gained {y}border <character>{/}, nested {y}suboption{/} blocks and a flat form such as {y}option 2 suboption 1 label Tools{/} (repeat suboption to go deeper). A line like {y}save --name.exe--C:\folder{/} now compiles straight to an EXE.
)BMG_NOTES",
LR"BMG_NOTES([g] 7. BUILT-IN CONSOLE TEXT EDITOR
A real full-screen editor now lives inside the program. It replaces the bare-bones Command Mode of 1.4.3 and is used by Text Mode, by the script editor for descriptions and command lists, and by {y}bmg -edit{/} on any plain text file.
  - Selection with Shift+Arrows or Ctrl+A. Clipboard: Ctrl+X / Ctrl+V, Ctrl+Insert (copy), Shift+Delete (cut), Shift+Insert (paste).
  - Undo/redo with Ctrl+Z / Ctrl+Y (200 steps). Find with Ctrl+F, repeat with F3.
  - Move and delete by word with Ctrl+Arrows and Ctrl+Backspace / Ctrl+Delete. Ctrl+Home / Ctrl+End jump to the start or end. Tab inserts four spaces.
  - Ctrl+S saves ("Save As" for unsaved text). A status bar shows line, column and selection. Esc opens a menu: Continue, Compile (finish), Exit to Main Menu or Exit without Save.

[g] 8. COMMAND-LINE SWITCHES
The program now understands these switches (a / works in place of the -):
  - {y}bmg -cli{/}  opens the built-in editor right away.
  - {y}bmg -config "file.txt"{/}  loads a text config and goes straight to generating the menu.
  - {y}bmg -edit "file"{/}  opens a generated .bat script in the Batch Script Editor, or any other text file in the built-in editor.
  - {y}bmg -compile "script.bat"{/}  builds script.exe next to the script with no menus. Exit code 0 = built, 1 = compile failed, 2 = bad usage or file not found, so it is safe to call from other scripts.
Wrong switches print a usage line instead of starting the program. The short {y}bmg{/} name comes from feature 9; without it, run BatchMenuGenerator.exe with the same switches.

[g] 9. SET UP PROGRAM AND FIRST-TIME SETUP
The first launch offers an optional quick setup; it is always available later under Settings > Set Up Program.
  - {y}Move program to AppData{/}: copies BatchMenuGenerator.exe to %LOCALAPPDATA%\BatchMenuGenerator, restarts from there and removes the original. Existing shortcuts and bmg commands are re-pointed.
  - {y}Create desktop shortcut{/} and {y}Add program to Start menu (Search){/}.
  - {y}Install / Update / Uninstall BMG commands{/}: adds the {y}bmg{/} command (a symlink, or a small bmg.cmd shim when symlinks are not allowed) in the AppData commands folder and puts that folder on your user PATH. No administrator rights needed; open a new terminal afterwards.
  - {y}Tips{/}: the program is portable, so moving it after creating shortcuts or commands can break them.

[g] 10. ALWAYS OPENS IN THE CLASSIC CONSOLE
When Windows Terminal is the default terminal it draws the menus incorrectly and hides the program icon. BatchMenuGenerator now notices when it was started inside a modern terminal and restarts itself in the classic console window (conhost.exe). No administrator rights needed.
  - Expect a brief flash of a Terminal window first: Windows opens it before the program gets to run.
  - -compile, wrong arguments, piped input, SSH sessions and debugger runs are never restarted. If conhost.exe cannot be started the program simply keeps running where it is.
  - To stay in the current terminal on purpose, start the program with {y}--bmg-conhost{/}.
)BMG_NOTES",
LR"BMG_NOTES([y] ======================== SMALLER UPGRADES ========================

[c] --- Generated scripts ---
  - Menu text is escaped everywhere. Option labels, the window title and custom border characters that contain & | < > ^ or % no longer break the script (1.4.3 wrote them as typed), and long labels are no longer cut off by a fixed-size buffer.
  - Pressing Enter on an empty prompt no longer re-runs the previous choice: the choice variable is cleared before every prompt.
  - Menu choices are matched case-insensitively.
  - Administrator relaunch now waits for the elevated window instead of the first window closing immediately.
  - The title is optional. Leave it empty and no title line or banner text is written, which lets the description work as a free-form banner. Description lines keep their leading spaces, so multi-line ASCII art stays aligned.

[c] --- Text configs ---
  - An {y}option N{/} block beyond the {y}option num{/} you declared now extends the option list (1.4.3 ignored it). Options without a label are called "Option".
  - Description lines keep their indentation (1.4.3 trimmed it).
  - The {y}border{/} directive chooses the border character for the generated menu.

[c] --- Wizard, editors and everyday use ---
  - Live title preview: the banner is drawn centered while you type. The 57-character limit is enforced as you type. Esc at the title step returns to the main menu.
  - The number of options is typed as digits only (1-20); Enter keeps the default of 3.
  - Most of the wizard's menus include an Exit entry. While you are still answering questions your progress is autosaved, so you can leave and resume later.
  - Save conflicts are menus now: destination folder missing (Retype file details / Create folder automatically), file already exists (Replace / Rename to <name>1.bat / Retype) and failed save (Retry / Use Explorer dialog), each with Exit & Save.
  - After saving you are asked "Compile this batch into an EXE?" and then "What would you like to do next?" (Main menu / Launch script / Exit). The End Action setting can answer the second question for you.
  - The Saveload and Menuborder settings skip the save/load-method and border questions.
  - The password step now points to the built-in EXE compiler instead of an external converter, and says plainly that a compiled EXE is obfuscated, not secured.
  - The window title is now "BatchMenuGenerator".
  - Warnings about invalid settings.txt values appear on the main screen.

[c] --- AutoSave ---
  - AutoSave can be switched off in Settings and understands sub menus.
  - The autosave is kept until the finished script is saved (or you decide not to save it), so even "Exit & Save" at the border or save prompts can be resumed on the next launch.
  - A resumed session continues through the normal flow: the same menus, the same border choice and the same Saveload / End Action behavior as a fresh one.

[c] --- This screen ---
  - The version history is built into the program, so a portable copy always has it (no extra file needed). It follows the active theme, wraps to the window width and pages with the keyboard: Down/Space for the next page, Up/Backspace for the previous one, Home/End for the first/last, Esc to close.
)BMG_NOTES",
LR"BMG_NOTES([y] ======================== WORKS DIFFERENTLY ========================
  - Main menu: 1 Make new Batch Menu, 2 Text Mode (was "Load TXT config"), 3 Edit existing generated .bat script (was the hidden Command Mode), S Settings, 0 Exit Program.
  - An empty title now means "no title" (1.4.3 used "Batch Menu").
  - Interactive launches always end up in the classic console window (feature 10).

[y] ======================== COMPATIBILITY ========================
  - Text configs written for 1.4.x load unchanged. The only differences: options without a label are called "Option", and description indentation is kept.
  - Scripts generated by 1.4.x can be opened and edited with "Edit existing generated .bat script".
  - Run modes 0 and 1 generate the same commands as 1.4.3, so existing menus behave the same.

[w] This list covers everything since 1.4.3; earlier changes were not recorded inside the program.
)BMG_NOTES",
};

static bool IsVersionColorTag(const std::wstring& name) {
    std::wstring tag = name;
    std::transform(tag.begin(), tag.end(), tag.begin(), ::towlower);
    return tag == L"cyan" || tag == L"c" || tag == L"header" ||
        tag == L"yellow" || tag == L"y" || tag == L"prompt" ||
        tag == L"green" || tag == L"g" || tag == L"success" ||
        tag == L"red" || tag == L"r" || tag == L"error" ||
        tag == L"white" || tag == L"w" || tag == L"text" ||
        tag == L"magenta" || tag == L"m" || tag == L"info" ||
        tag == L"/" || tag == L"reset";
}

static bool SplitVersionLinePrefix(const std::wstring& line, std::wstring& prefix, std::wstring& body) {
    static const wchar_t* const prefixes[] = {
        L"[cyan] ", L"[c] ", L"[yellow] ", L"[y] ", L"[green] ", L"[g] ",
        L"[red] ", L"[r] ", L"[magenta] ", L"[m] ", L"[white] ", L"[w] "
    };
    for (const wchar_t* candidate : prefixes) {
        const std::wstring known = candidate;
        if (line.compare(0, known.size(), known) == 0) {
            prefix = known;
            body = line.substr(known.size());
            return true;
        }
    }
    prefix.clear();
    body = line;
    return false;
}

static int VersionVisibleWidth(const std::wstring& text) {
    int width = 0;
    for (size_t i = 0; i < text.size();) {
        if (text[i] == L'{') {
            const size_t close = text.find(L'}', i);
            if (close != std::wstring::npos && IsVersionColorTag(text.substr(i + 1, close - i - 1))) {
                i = close + 1;
                continue;
            }
        }
        ++width;
        ++i;
    }
    return width;
}

static void WrapVersionLine(const std::wstring& line, int width, std::vector<std::wstring>& out) {
    if (width < 20) width = 20;

    std::wstring prefix, body;
    SplitVersionLinePrefix(line, prefix, body);
    while (!body.empty() && (body.back() == L' ' || body.back() == L'\t' || body.back() == L'\r')) body.pop_back();
    if (body.empty()) {
        out.push_back(prefix);
        return;
    }

    size_t lead = 0;
    while (lead < body.size() && body[lead] == L' ') ++lead;
    size_t hang = lead;
    if (body.compare(lead, 2, L"- ") == 0) hang = lead + 2;
    if ((int)hang > width / 2) hang = 0;
    const std::wstring hangText(hang, L' ');

    std::wstring row;
    int col = 0;
    size_t lastSpace = std::wstring::npos;
    std::wstring tagAtSpace, activeTag;
    bool wroteRow = false;
    bool rowHasText = false;

    auto emit = [&](std::wstring text) {
        while (!text.empty() && text.back() == L' ') text.pop_back();
        out.push_back(prefix + text);
        wroteRow = true;
    };
    auto startContinuation = [&](const std::wstring& carriedText, const std::wstring& tag) {
        row = hangText + tag + carriedText;
        col = (int)hang + VersionVisibleWidth(carriedText);
        lastSpace = std::wstring::npos;
        rowHasText = false;
    };

    for (size_t i = 0; i < body.size();) {
        if (body[i] == L'{') {
            const size_t close = body.find(L'}', i);
            if (close != std::wstring::npos) {
                const std::wstring name = body.substr(i + 1, close - i - 1);
                if (IsVersionColorTag(name)) {
                    const std::wstring tagText = body.substr(i, close - i + 1);
                    std::wstring lowered = name;
                    std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::towlower);
                    row += tagText;
                    activeTag = (lowered == L"/" || lowered == L"reset") ? std::wstring() : tagText;
                    i = close + 1;
                    continue;
                }
            }
        }

        const wchar_t ch = body[i++];

        if (ch == L' ') {
            if (wroteRow && !rowHasText) continue;
            if (col >= width) {
                emit(row);
                startContinuation(L"", activeTag);
                continue;
            }
            row += ch;
            ++col;
            if (rowHasText) {
                lastSpace = row.size() - 1;
                tagAtSpace = activeTag;
            }
            continue;
        }

        if (col >= width) {
            if (lastSpace != std::wstring::npos) {
                const std::wstring head = row.substr(0, lastSpace);
                const std::wstring tail = row.substr(lastSpace + 1);
                emit(head);
                startContinuation(tail, tagAtSpace);
            } else {
                emit(row);
                startContinuation(L"", activeTag);
            }
            while (col >= width) {
                emit(row);
                startContinuation(L"", activeTag);
            }
        }
        row += ch;
        ++col;
        rowHasText = true;
    }

    if (rowHasText || !wroteRow) emit(row);
}

static std::vector<std::wstring> BuildPatchNoteLines() {
    std::vector<std::wstring> lines;
    for (const wchar_t* chunk : kPatchNotes) {
        const std::wstring text = chunk;
        size_t start = 0;
        while (start <= text.size()) {
            const size_t end = text.find(L'\n', start);
            std::wstring line = text.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            const std::wstring trimmed = Trim(line);
            if (!(trimmed.size() >= 2 && trimmed.compare(0, 2, L"//") == 0)) lines.push_back(line);
            if (end == std::wstring::npos) break;
            start = end + 1;
        }
    }
    while (!lines.empty() && Trim(lines.back()).empty()) lines.pop_back();
    return lines;
}

static bool IsVersionHeadingRow(const std::wstring& row) {
    return row.compare(0, 4, L"[c] ") == 0 || row.compare(0, 4, L"[y] ") == 0 || row.compare(0, 4, L"[g] ") == 0;
}

static std::vector<std::pair<size_t, size_t>> PaginateVersionRows(const std::vector<std::wstring>& rows, size_t pageHeight) {
    std::vector<std::pair<size_t, size_t>> pages;
    if (pageHeight < 1) pageHeight = 1;
    size_t start = 0;
    while (start < rows.size() && rows[start].empty()) ++start;
    while (start < rows.size()) {
        size_t end = (std::min)(start + pageHeight, rows.size());
        while (end < rows.size() && end - start > 1 && IsVersionHeadingRow(rows[end - 1])) --end;
        pages.push_back(std::make_pair(start, end));
        start = end;
        while (start < rows.size() && rows[start].empty()) ++start;
    }
    if (pages.empty()) pages.push_back(std::make_pair((size_t)0, (size_t)0));
    return pages;
}

enum class VersionNavKey { Close, Advance, Next, Prev, First, Last };

static VersionNavKey ReadVersionNavKey() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    while (true) {
        INPUT_RECORD input{};
        DWORD eventsRead = 0;
        if (!ReadConsoleInputW(hIn, &input, 1, &eventsRead)) return VersionNavKey::Close;
        if (eventsRead == 0 || input.EventType != KEY_EVENT || !input.Event.KeyEvent.bKeyDown) continue;

        const WORD key = input.Event.KeyEvent.wVirtualKeyCode;
        if (key == VK_ESCAPE || key == 'Q') return VersionNavKey::Close;
        if (key == VK_SPACE || key == VK_RETURN) return VersionNavKey::Advance;
        if (key == VK_DOWN || key == VK_RIGHT || key == VK_NEXT) return VersionNavKey::Next;
        if (key == VK_UP || key == VK_LEFT || key == VK_PRIOR || key == VK_BACK) return VersionNavKey::Prev;
        if (key == VK_HOME) return VersionNavKey::First;
        if (key == VK_END) return VersionNavKey::Last;
    }
}

static void RunVersionHistoryMenu() {
    const std::vector<std::wstring> noteLines = BuildPatchNoteLines();

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo{};
    GetConsoleCursorInfo(hOut, &cursorInfo);
    const BOOL cursorWasVisible = cursorInfo.bVisible;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);

    size_t page = 0;
    while (true) {
        int columns = 100, rowsAvailable = 30;
        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
            columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            rowsAvailable = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
        const int textWidth = (std::max)(30, (std::min)(columns - 2, 110));
        const size_t pageHeight = (size_t)(std::max)(5, rowsAvailable - 5);

        std::vector<std::wstring> wrapped;
        for (const auto& line : noteLines) WrapVersionLine(line, textWidth, wrapped);
        const std::vector<std::pair<size_t, size_t>> pages = PaginateVersionRows(wrapped, pageHeight);
        if (page >= pages.size()) page = pages.size() - 1;

        system("cls");
        C_H; wprintf(L"BatchMenuGenerator %ls  -  Patch Notes\n\n", kBmgVersionLabel); C_R;
        for (size_t r = pages[page].first; r < pages[page].second; ++r) RenderVersionLine(wrapped[r]);

        const std::wstring counter = L"Page " + std::to_wstring(page + 1) + L"/" + std::to_wstring(pages.size());
        std::wstring footer = counter + L"  |  Down/Space: next  |  Up/Backspace: previous  |  Home/End  |  Esc: close";
        if ((int)footer.size() > columns - 1) footer = counter + L"  |  Up/Down  |  Esc: close";
        if ((int)footer.size() > columns - 1) footer.resize((size_t)(std::max)(1, columns - 1));
        wprintf(L"\n");
        C_P; wprintf(L"%ls", footer.c_str()); C_R;

        size_t newPage = page;
        bool closeViewer = false;
        while (true) {
            const VersionNavKey key = ReadVersionNavKey();
            if (key == VersionNavKey::Close) { closeViewer = true; break; }
            if (key == VersionNavKey::Advance) {
                if (page + 1 >= pages.size()) { closeViewer = true; break; }
                newPage = page + 1; break;
            }
            if (key == VersionNavKey::Next && page + 1 < pages.size()) { newPage = page + 1; break; }
            if (key == VersionNavKey::Prev && page > 0) { newPage = page - 1; break; }
            if (key == VersionNavKey::First && page != 0) { newPage = 0; break; }
            if (key == VersionNavKey::Last && page + 1 != pages.size()) { newPage = pages.size() - 1; break; }
        }
        if (closeViewer) break;
        page = newPage;
    }

    cursorInfo.bVisible = cursorWasVisible;
    SetConsoleCursorInfo(hOut, &cursorInfo);
}

static bool RunMode1Flow(std::wstring& title, std::vector<std::wstring>& description,
    std::wstring& password, std::vector<OptionData>& options, bool& requireAdmin,
    wchar_t& borderChar, const std::wstring& resumeStep = L"") {

    int resumeStage = 0;
    size_t resumeOptIndex = 0;

    if (resumeStep == L"TITLE_DONE") {
        resumeStage = 1;
    } else if (resumeStep == L"DESC_LINE" || resumeStep == L"DESC_DONE") {
        resumeStage = 2;
    } else if (resumeStep == L"PASS_DONE") {
        resumeStage = 3;
    } else if (resumeStep == L"ADMIN_DONE") {
        resumeStage = 4;
    } else if (resumeStep == L"OPTCOUNT_DONE") {
        resumeStage = 5;
        resumeOptIndex = 0;
    } else if (resumeStep.find(L"OPT_") == 0) {
        resumeStage = 5;
        if (resumeStep.find(L"OPT_DONE_") == 0) {
            int idx = _wtoi(resumeStep.substr(9).c_str());
            resumeOptIndex = (idx >= 0) ? (size_t)(idx + 1) : 0;
        } else if (resumeStep.find(L"OPT_LABEL_") == 0) {
            int idx = _wtoi(resumeStep.substr(10).c_str());
            resumeOptIndex = (idx >= 0) ? (size_t)idx : 0;
        } else if (resumeStep.find(L"OPT_RUNMODE_") == 0) {
            int idx = _wtoi(resumeStep.substr(12).c_str());
            resumeOptIndex = (idx >= 0) ? (size_t)idx : 0;
        } else if (resumeStep.find(L"OPT_CMD_") == 0) {
            int idx = _wtoi(resumeStep.substr(8).c_str());
            resumeOptIndex = (idx >= 0) ? (size_t)idx : 0;
        }
    } else if (resumeStep == L"ALL_OPTIONS_DONE") {
        resumeStage = 6;
        resumeOptIndex = options.size();
    }

    std::wstring sessionTimestamp;
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t buf[64];
        swprintf(buf, 64, L"%02d:%02d %02d/%02d/%02d",
            st.wHour, st.wMinute, st.wDay, st.wMonth, st.wYear % 100);
        sessionTimestamp = buf;
    }

    auto SaveStep = [&](const std::wstring& stepCode) {
        if (!g_appConfig.autosave) return;

        std::vector<std::wstring> lines;
        lines.push_back(L"// Autosave from " + sessionTimestamp);
        lines.push_back(L"step " + stepCode);
        lines.push_back(L"title " + title);
        for (const auto& d : description) lines.push_back(L"description " + d);
        if (!password.empty()) lines.push_back(L"pass " + password);
        lines.push_back(requireAdmin ? L"admin y" : L"admin n");
        lines.push_back(L"option num: " + std::to_wstring(options.size()));

        std::function<void(const OptionData&, const std::wstring&, int)> writeOption;
        writeOption = [&](const OptionData& opt, const std::wstring& prefix, int depth) {
            if (depth > 8) return;
            lines.push_back(prefix + L" {");
            lines.push_back(L"runmode " + std::to_wstring(opt.runMode));

            if (opt.runMode == 2) {
                lines.push_back(L"suboption num: " + std::to_wstring(opt.subOptions.size()));
                for (size_t si = 0; si < opt.subOptions.size(); ++si) {
                    std::wstring n = std::to_wstring(si + 1);
                    if (!opt.subOptions[si].label.empty())
                        lines.push_back(L"suboption " + n + L" label " + opt.subOptions[si].label);
                    writeOption(opt.subOptions[si], L"suboption " + n, depth + 1);
                }
            } else {
                for (const auto& c : opt.commands) lines.push_back(c);
            }
            lines.push_back(L"}");
        };

        for (size_t oi = 0; oi < options.size(); ++oi) {
            std::wstring optNum = std::to_wstring(oi + 1);
            if (!options[oi].label.empty())
                lines.push_back(L"option " + optNum + L" label " + options[oi].label);
            writeOption(options[oi], L"option " + optNum, 0);
        }
        WriteAutoSave(lines);
    };

    if (resumeStage < 1) {
        std::wstring enteredTitle = ReadTitleWithPreview(title);
        if (enteredTitle == L"__BMG_CANCEL__") return false;
        title = enteredTitle;
        SaveStep(L"TITLE_DONE");
    }

    if (resumeStage < 2) {
        bool wantDesc = true;
        if (g_enableTUI) {
            int descChoice = RunTUIMenu(L"Add Description?", {L"Yes, add description", L"No, continue", L"Exit"}, 0, -1, false);
            if (descChoice == -1 || descChoice == 2) exit(0);
            wantDesc = (descChoice == 0);
        } else {
            C_P; wprintf(L"  Do you want to add a description? (Y/n) [X to exit]: "); C_I;
            std::wstring descAns = ReadLine(); C_R;
            if (descAns == L"X" || descAns == L"x") exit(0);
            wantDesc = (descAns.empty() || descAns == L"Y" || descAns == L"y" || descAns == L"yes");
        }
        if (wantDesc) {
            C_P; wprintf(L"  Enter description lines below.\n");
            wprintf(L"  Use this area for multiline ASCII art when no text title is wanted.\n");
            wprintf(L"  Press Enter on an empty line when done.\n"); C_R;

            while (true) {
                C_I; wprintf(L"    > "); std::wstring descLine = ReadLine(); C_R;
                if (descLine.empty()) break;
                description.push_back(descLine);
                SaveStep(L"DESC_LINE");
            }
        }
        SaveStep(L"DESC_DONE");
    }

    if (resumeStage < 3) {
        C_P; wprintf(L"\n  [Note] A password stored in a .bat file can be exposed by editing the script.\n");
        wprintf(L"  Use EXE compilation to make the password option harder to inspect and effective during normal execution.\n");
        wprintf(L"  The compiled EXE obfuscates the embedded batch source, but this is not a cryptographic security boundary.\n");
        wprintf(L"  Enter password (leave empty for none): "); C_I;
        password = ReadLine(); C_R;
        SaveStep(L"PASS_DONE");
    }

    if (resumeStage < 4) {
        if (g_enableTUI) {
            int adminChoice = RunTUIMenu(L"Administrative Privileges?", {L"Yes - Require administrator privileges", L"No - Standard user privileges", L"Exit"}, requireAdmin ? 0 : 1, -1, false);
            if (adminChoice == -1 || adminChoice == 2) exit(0);
            requireAdmin = (adminChoice == 0);
        } else {
            C_P; wprintf(L"\n  Will this script require Administrative privileges? (y/N) [X to exit]: "); C_I;
            std::wstring adminAns = ReadLine(); C_R;
            if (adminAns == L"X" || adminAns == L"x") exit(0);
            requireAdmin = (!adminAns.empty() && (adminAns == L"y" || adminAns == L"Y" || adminAns == L"yes"));
        }
        if (requireAdmin) {
            C_S; wprintf(L"  [Admin elevation code will be added to the script]\n"); C_R;
        }
        SaveStep(L"ADMIN_DONE");
    }

    if (resumeStage < 5) {
        int optionCount = 3;
        while (true) {
            C_P; wprintf(L"  Number of options (1-20) [3]: "); C_I;
            std::wstring numStr = ReadOptionCountInput(); C_R;
            if (numStr.empty()) {
                optionCount = 3;
                break;
            }
            wchar_t* end = nullptr;
            int val = (int)wcstol(numStr.c_str(), &end, 10);
            if (end && *end == L'\0' && val >= 1 && val <= 20) {
                optionCount = val;
                break;
            }
            C_E; wprintf(L"  [Error] Invalid number. Please enter a value between 1 and 20.\n"); C_R;
        }
        options.resize((size_t)optionCount);
        SaveStep(L"OPTCOUNT_DONE");
    }

    for (size_t i = resumeOptIndex; i < options.size(); ++i) {
        C_N; wprintf(L"\n  --- Option %zu ---\n", i + 1); C_R;

        C_P;
        if (!options[i].label.empty())
            wprintf(L"  Label [Option %zu] [%ls]: ", i + 1, options[i].label.c_str());
        else
            wprintf(L"  Label [Option %zu]: ", i + 1);
        C_I;
        std::wstring label = ReadLine(); C_R;
        if (!label.empty()) options[i].label = label;
        if (options[i].label.empty()) options[i].label = L"Option";
        SaveStep(L"OPT_LABEL_" + std::to_wstring(i));

        options[i].runMode = PromptRunMode(options[i].runMode, true);
        SaveStep(L"OPT_RUNMODE_" + std::to_wstring(i));

        if (options[i].runMode == 2) {
            options[i].commands.clear();
            ConfigureSubmenuInteractive(options[i].subOptions);
        } else {
            options[i].subOptions.clear();

            if (!options[i].commands.empty()) {
                C_S; wprintf(L"  Existing commands for Option %zu:\n", i + 1);
                for (const auto& c : options[i].commands) wprintf(L"    > %ls\n", c.c_str());
                C_R;
                C_P; wprintf(L"  Add more command(s), one per line (empty line when done):\n"); C_R;
            } else {
                C_P; wprintf(L"  Enter command(s), one per line.\n  Press Enter on an empty line when done.\n"); C_R;
            }

            while (true) {
                C_I; wprintf(L"    > "); std::wstring cmd = ReadLine(); C_R;
                if (cmd.empty()) break;
                options[i].commands.push_back(cmd);
                SaveStep(L"OPT_CMD_" + std::to_wstring(i));
            }
        }

        SaveStep(L"OPT_DONE_" + std::to_wstring(i));
    }

    SaveStep(L"ALL_OPTIONS_DONE");
    return true;
}

struct TesterAction {
    wchar_t key;
    const wchar_t* label;
    void (*action)();
};

static void TaGenBatchAll() {
    C_S; wprintf(L"\n  [TEST] Generating batch with all features...\n"); C_R;
    wchar_t bc = L'=';
    std::wstring title = L"Test Menu";
    std::vector<std::wstring> desc = { L"Test description line 1", L"Test description line 2" };
    std::vector<OptionData> opts(3);
    opts[0].label = L"Run Notepad";
    opts[0].commands.push_back(L"notepad.exe");
    opts[1].label = L"Run Calc";
    opts[1].commands.push_back(L"calc.exe");
    opts[2].label = L"Show IP";
    opts[2].commands.push_back(L"ipconfig");
    std::wstring script = GenerateBatch(title, desc, opts, L"test123", true, bc);
    C_B; wprintf(L"\n  ========== GENERATED SCRIPT ==========\n"); C_R;
    wprintf(L"%ls\n", script.c_str());
    system("pause");
}

static void TaGenBatchAdmin() {
    C_S; wprintf(L"\n  [TEST] Generating batch with admin check only...\n"); C_R;
    wchar_t bc = L'=';
    std::wstring title = L"Admin Test";
    std::vector<OptionData> opts(2);
    opts[0].label = L"Run Notepad";
    opts[0].commands.push_back(L"notepad.exe");
    opts[1].label = L"Exit";
    opts[1].commands.push_back(L"exit");
    std::wstring script = GenerateBatch(title, {}, opts, L"", true, bc);
    C_B; wprintf(L"\n  ========== GENERATED SCRIPT ==========\n"); C_R;
    wprintf(L"%ls\n", script.c_str());
    system("pause");
}

static void TaGenBatchPassword() {
    C_S; wprintf(L"\n  [TEST] Generating batch with password only...\n"); C_R;
    wchar_t bc = L'-';
    std::wstring title = L"Password Protected";
    std::vector<OptionData> opts(1);
    opts[0].label = L"Run Notepad";
    opts[0].commands.push_back(L"notepad.exe");
    std::wstring script = GenerateBatch(title, {}, opts, L"secret123", false, bc);
    C_B; wprintf(L"\n  ========== GENERATED SCRIPT ==========\n"); C_R;
    wprintf(L"%ls\n", script.c_str());
    system("pause");
}

static void TaGenBatchBorder() {
    C_S; wprintf(L"\n  [TEST] Generating batch with custom zigzag border...\n"); C_R;
    wchar_t bc = L'Z';
    std::wstring title = L"Zigzag Border Test";
    std::vector<OptionData> opts(1);
    opts[0].label = L"Run Notepad";
    opts[0].commands.push_back(L"notepad.exe");
    std::wstring script = GenerateBatch(title, {}, opts, L"", false, bc);
    C_B; wprintf(L"\n  ========== GENERATED SCRIPT ==========\n"); C_R;
    wprintf(L"%ls\n", script.c_str());
    system("pause");
}

static void TaSaveToManualPath() {
    C_S; wprintf(L"\n  [TEST] SaveToManualPath test...\n"); C_R;
    std::wstring script = L"@echo off\r\necho Hello from BatchMenuGenerator!\r\npause\r\n";
    std::wstring savedPath;
    SaveToManualPath(script, savedPath, L"BMG_TestScript", GetExePath(), false);
    system("pause");
}

static void TaParseConfig() {
    C_S; wprintf(L"\n  [TEST] ParseScriptConfig test...\n"); C_R;
    std::vector<std::wstring> testLines = {
        L"title Test Config",
        L"description This is a test config",
        L"admin y",
        L"pass mypassword",
        L"option num: 2",
        L"option 1 label Notepad",
        L"option 1 {",
        L"notepad.exe",
        L"}",
        L"option 2 label Calculator",
        L"option 2 {",
        L"calc.exe",
        L"}"
    };
    std::wstring title;
    std::vector<std::wstring> desc;
    std::wstring pass;
    std::vector<OptionData> opts;
    std::wstring autoFn, autoPath;
    bool silent = false, admin = false;
    if (ParseScriptConfigLines(testLines, title, desc, pass, opts, autoFn, autoPath, silent, admin)) {
        C_S; wprintf(L"  [SUCCESS] Parsed config!\n"); C_R;
        C_P; wprintf(L"  Title: "); C_I; wprintf(L"%ls\n", title.c_str()); C_R;
        C_P; wprintf(L"  Password: "); C_I; wprintf(L"%ls\n", pass.c_str()); C_R;
        C_P; wprintf(L"  Require Admin: "); C_I; wprintf(L"%s\n", admin ? L"yes" : L"no"); C_R;
        for (size_t i = 0; i < opts.size(); i++) {
            C_P; wprintf(L"  Option %zu: ", i + 1); C_I; wprintf(L"%ls\n", opts[i].label.c_str()); C_R;
            C_P; wprintf(L"    RunMode: "); C_I; wprintf(L"%d\n", opts[i].runMode); C_R;
            for (const auto& cmd : opts[i].commands) {
                C_P; wprintf(L"    Cmd: "); C_I; wprintf(L"%ls\n", cmd.c_str()); C_R;
            }
        }
    } else {
        C_E; wprintf(L"  [ERROR] Failed to parse config.\n"); C_R;
    }
    system("pause");
}

static void TaRunMode1Flow() {
    C_S; wprintf(L"\n  [TEST] Running Mode 1 flow (with autosave steps)...\n"); C_R;
    std::wstring title = L"Auto Test";
    std::vector<std::wstring> desc;
    std::wstring pass;
    std::vector<OptionData> opts;
    bool admin = false;
    wchar_t bc = L'=';
    bool completed = RunMode1Flow(title, desc, pass, opts, admin, bc);
    C_S; wprintf(L"\n  [TEST] Flow completed: %s\n", completed ? L"yes" : L"no"); C_R;
    C_P; wprintf(L"  Title: "); C_I; wprintf(L"%ls\n", title.c_str()); C_R;
    C_P; wprintf(L"  Options: "); C_I; wprintf(L"%zu\n", opts.size()); C_R;
    system("pause");
}

static const TesterAction g_testerActions[] = {
    { L'1', L"Generate batch with ALL features", TaGenBatchAll },
    { L'2', L"Generate batch with ADMIN check only", TaGenBatchAdmin },
    { L'3', L"Generate batch with PASSWORD only", TaGenBatchPassword },
    { L'4', L"Generate batch with CUSTOM BORDER (ZigZag)", TaGenBatchBorder },
    { L'5', L"Run SaveToManualPath test", TaSaveToManualPath },
    { L'6', L"Run ParseScriptConfig test", TaParseConfig },
    { L'7', L"Run Mode 1 Flow (autosave steps)", TaRunMode1Flow }
};

static void RunTesterMenu() {
    while (true) {
        system("cls");
        C_B; TuiBannerBorder();
        C_N; TuiBannerTitle(L"TESTER MENU - PROGRAM ACTIONS");
        C_B; TuiBannerBorder();
        wprintf(L"\n"); C_T;
        wprintf(L"  Select an action to test/launch:\n\n");
        for (const auto& a : g_testerActions) {
            C_P; wprintf(L"    %lc) ", (wint_t)a.key);
            if (a.key >= L'0' && a.key <= L'9') C_I; else C_A;
            wprintf(L"%ls\n", a.label);
        }
        C_T; wprintf(L"\n    Q) Return to Debug Menu\n\n");
        C_P; wprintf(L"  Select option: "); C_I;
        int tc = _getwch(); C_T;
        if (tc == L'Q' || tc == L'q' || tc == 27) break;
        bool found = false;
        for (const auto& a : g_testerActions) {
            if (tc == a.key) {
                found = true;
                a.action();
                break;
            }
        }
        if (!found) {
            C_E; wprintf(L"\n  [Error] Invalid option.\n"); C_R;
            Sleep(800);
        }
    }
    C_R; system("cls");
}

static void RunDebugMenu() {
    system("cls");
    C_B; TuiBannerBorder();
    C_N; TuiBannerTitle(L"DEBUG MODE - TESTING MENU");
    C_B; TuiBannerBorder();
    wprintf(L"\n"); C_T;
    C_P; wprintf(L"    1) Settings (bconfig) toggle : "); C_I; wprintf(L"%s\n", g_appConfig.bconfig ? L"ENABLED" : L"disabled"); C_T;
    C_P; wprintf(L"    2) Launch Tester\n\n");
    C_P; wprintf(L"  Select option (ESC/0 to exit): "); C_I;
    int choice = _getwch(); C_T;
    if (choice == L'1') {
        g_appConfig.bconfig = (g_appConfig.bconfig == 1) ? 0 : 1;
        if (g_appConfig.bconfig == 1) {
            std::vector<std::wstring> warnings;
            LoadSettingsFile(g_appConfig, warnings);
        }
        RunDebugMenu(); return;
    }
    if (choice == L'2') { RunTesterMenu(); RunDebugMenu(); return; }
    C_R; system("cls");
}

static void ShowSyntaxTutorial() {
    C_H; wprintf(L"  --- Syntax Tutorial ---\n\n"); C_R;
    static const wchar_t* kSyntaxTutorial = LR"BMG_TUTORIAL(// SYNTAX: title [Your Menu Title Here] (Max 57 characters)
// EXAMPLE: title Admin Toolbox
// This field is not required, leave the title line out for a menu without a title (handy for ASCII art in the description)
title [your title here]

// SYNTAX: description
// Note: You can repeat this keyword on new lines to add multiple description paragraphs or multiline ASCII art.
// EXAMPLE: description Version 1.0 - Use with administrator privileges.
// This field is not required, simply don't add the description line to not include
description 

// SYNTAX: pass [Password]
// Note: Leave blank, or use 'n' or 'no' if you do not want a password.
// EXAMPLE: pass MySecurePass123
// This field is not required
pass 

// SYNTAX: admin [y or n]
// Note: Type 'admin y' to force the script to request administrative privileges upon launch.
// If not written, or set to 'n', it runs with standard user privileges unless manualy launched as admin.
admin y/n

// CRITICAL: You MUST define this total number BEFORE writing individual option details!
// SYNTAX: option num: [number from 1 to 20]
// EXAMPLE: option num: 2
// This field is required
option num: [the number of options you want to have]

// Repeat the structure below for every option up to your total option number.

// --- OPTION 1 ---
// SYNTAX: option [Number] label [Menu Display Name]
// EXAMPLE: option 1 label Check System Information
// This is required for every option up to the limit you set
option 1 label [your label]

// SYNTAX: option [Number] { [Your Batch Commands] }
// Note: Put every batch command on its own separate line inside the brackets.
// Note: Leave empty if runmode is set to 2 (Submenu).
option 1 {
    [commands go here inside the brackets, in separate lines]
}

// SYNTAX: runmode [0, 1, or 2]
// Note: Place it immediately after the closing bracket '}' (or on its own line inside the brackets).
// 0 = Direct Call (call your_command)
// 1 = CMD Wrapper (cmd /c "your_command")
// 2 = Submenu (creates a nested sub-menu section)
// If omitted, runmode defaults to 0
runmode [for direct cmd commands choose 0, for .exe files with paths choose 1, for submenus choose 2]


// --- OPTION 2 (EXAMPLE: SUBMENU / RUNMODE 2) ---
option 2 label [your submenu label]
option 2 {
    [leave empty for submenus]
}
runmode 2

// For Submenus (runmode 2), define nested sub-options (1-20 per submenu, max depth 6):
// SYNTAX: option [Parent] suboption [Number] label
// SYNTAX: option [Parent] suboption [Number] { [Submenu Batch Commands] }
// SYNTAX: option [Parent] suboption [Number] runmode [0, 1, or 2]
// To go deeper, repeat suboption: option 2 suboption 1 suboption 2 label [your label]
option 2 suboption 1 label [your sub-option label]
option 2 suboption 1 {
    [sub-option commands go here]
}
option 2 suboption 1 runmode 0


// SYNTAX: border [character]
// EXAMPLE: border *
// Use the first character after "border" for menu borders.
// If this line is omitted, the default border character is =
border =

// SYNTAX OPTIONS FOR 'save':
//   save n                         -> Preview the script only (will not write to disk)
//   save --[filename.bat]          -> Auto-saves to the folder where this tool runs 
//   save --[filename.bat]--[path]  -> Auto-saves to a specific target folder path
//   save --[filename.exe]--[path]  -> Compiles directly into a standalone executable EXE
// EXAMPLE: save --my_new_batch_menu.bat--C:\Users\User\Desktop
save [your preferred option]

// BONUS: To compile ANY .bat script into an .exe, in your terminal navigate to the program's location and type the command:
// BatchMenuGenerator.exe "path-to-bat-script.bat" -compile 
// OR
// BatchMenuGenerator.exe --path-to-bat-script.bat -compile)BMG_TUTORIAL";
    wprintf(L"%ls\n", kSyntaxTutorial);
    C_P; wprintf(L"\n  Press any key to return...\n"); C_R;
    _getwch();
}

static bool IsModernTerminalHost(bool interactiveConsole, bool onInteractiveDesktop,
    bool hasConsoleWindow, const std::wstring& consoleWindowClass,
    const std::wstring& ownerWindowClass, bool hasWtSession) {

    if (!interactiveConsole || !onInteractiveDesktop) return false;

    if (hasConsoleWindow) {
        if (consoleWindowClass == L"ConsoleWindowClass") return false;
        if (consoleWindowClass == L"PseudoConsoleWindow") return true;
        if (ownerWindowClass == L"CASCADIA_HOSTING_WINDOW_CLASS") return true;
    }

    return hasWtSession;
}

static bool IsHostedInModernTerminal() {
    if (IsDebuggerPresent()) return false;

    DWORD consoleMode = 0;
    const bool interactiveConsole =
        GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &consoleMode) &&
        GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &consoleMode);

    wchar_t stationName[64] = {0};
    DWORD stationBytes = 0;
    HWINSTA station = GetProcessWindowStation();
    const bool onInteractiveDesktop = station &&
        GetUserObjectInformationW(station, UOI_NAME, stationName, sizeof(stationName), &stationBytes) &&
        _wcsicmp(stationName, L"WinSta0") == 0;

    std::wstring consoleClass, ownerClass;
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow) {
        wchar_t className[64] = {0};
        if (GetClassNameW(consoleWindow, className, 64) > 0) consoleClass = className;
        HWND owner = GetAncestor(consoleWindow, GA_ROOTOWNER);
        if (owner && owner != consoleWindow) {
            wchar_t ownerName[64] = {0};
            if (GetClassNameW(owner, ownerName, 64) > 0) ownerClass = ownerName;
        }
    }

    wchar_t session[8] = {0};
    const bool hasWtSession = GetEnvironmentVariableW(L"WT_SESSION", session, 8) > 0;

    return IsModernTerminalHost(interactiveConsole, onInteractiveDesktop,
        consoleWindow != NULL, consoleClass, ownerClass, hasWtSession);
}

static std::wstring BuildClassicConsoleCommandLine(const std::wstring& conhostPath,
    const std::wstring& exePath, int argc, wchar_t* argv[]) {
    std::wstring command = QuoteCommandLineArgument(conhostPath) + L" " +
        QuoteCommandLineArgument(exePath) + L" --bmg-conhost";
    for (int i = 1; i < argc; ++i) command += L" " + QuoteCommandLineArgument(argv[i]);
    return command;
}

static bool RelaunchInClassicConsole(int argc, wchar_t* argv[]) {
    const std::wstring exePath = GetExecutableFilePath();
    if (exePath.empty()) return false;

    wchar_t systemDirectory[MAX_PATH] = {0};
    if (GetSystemDirectoryW(systemDirectory, MAX_PATH) == 0) return false;
    const std::wstring conhostPath = std::wstring(systemDirectory) + L"\\conhost.exe";
    if (GetFileAttributesW(conhostPath.c_str()) == INVALID_FILE_ATTRIBUTES) return false;

    const std::wstring command = BuildClassicConsoleCommandLine(conhostPath, exePath, argc, argv);
    std::vector<wchar_t> commandBuffer(command.begin(), command.end());
    commandBuffer.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(conhostPath.c_str(), commandBuffer.data(), NULL, NULL, FALSE, 0, NULL, NULL,
        &startup, &process)) {
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

int wmain(int argc, wchar_t* argv[]) {
    SetConsoleTitle(L"BatchMenuGenerator");
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    bool skipConsoleRelaunch = false;
    std::vector<wchar_t*> filteredArgs;
    for (int i = 0; i < argc; ++i) {
        if (i > 0 && _wcsicmp(argv[i], L"--bmg-conhost") == 0) {
            skipConsoleRelaunch = true;
            continue;
        }
        filteredArgs.push_back(argv[i]);
    }
    filteredArgs.push_back(NULL);
    argc = (int)filteredArgs.size() - 1;
    argv = filteredArgs.data();

    if (argc >= 4 && _wcsicmp(argv[1], L"--bmg-finish-move") == 0) {
        const DWORD oldProcessId = (DWORD)_wtoi(argv[3]);
        HANDLE oldProcess = oldProcessId ? OpenProcess(SYNCHRONIZE, FALSE, oldProcessId) : NULL;
        if (oldProcess) {
            WaitForSingleObject(oldProcess, INFINITE);
            CloseHandle(oldProcess);
        }
        if (!DeleteFileW(argv[2]) && GetFileAttributesW(argv[2]) != INVALID_FILE_ATTRIBUTES) {
            C_E; wprintf(L"  [Warning] AppData copy is running, but the old program could not be removed: %ls\n", argv[2]); C_R;
        } else {
            C_S; wprintf(L"  BatchMenuGenerator moved to AppData.\n"); C_R;
        }
        argc = 1;
    }

    bool compileFlag = false;
    int compilePathArg = -1;
    int startupMode = 0;
    std::wstring startupPath;
    bool invalidArguments = false;
    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (_wcsicmp(arg.c_str(), L"-compile") == 0 ||
            _wcsicmp(arg.c_str(), L"/compile") == 0 ||
            _wcsicmp(arg.c_str(), L"--compile") == 0) {
            compileFlag = true;
        } else if (_wcsicmp(arg.c_str(), L"-cli") == 0 || _wcsicmp(arg.c_str(), L"/cli") == 0) {
            if (startupMode != 0) invalidArguments = true;
            startupMode = 1;
        } else if (_wcsicmp(arg.c_str(), L"-config") == 0 || _wcsicmp(arg.c_str(), L"/config") == 0 ||
            _wcsicmp(arg.c_str(), L"-edit") == 0 || _wcsicmp(arg.c_str(), L"/edit") == 0) {
            const int requestedMode = (_wcsicmp(arg.c_str(), L"-config") == 0 || _wcsicmp(arg.c_str(), L"/config") == 0) ? 2 : 3;
            if (startupMode != 0 || i + 1 >= argc) {
                invalidArguments = true;
            } else {
                startupMode = requestedMode;
                startupPath = argv[++i];
                if (startupPath.empty()) invalidArguments = true;
            }
        } else if (compilePathArg == -1) {
            compilePathArg = i;
        } else {
            invalidArguments = true;
        }
    }

    if (invalidArguments || (compileFlag && startupMode != 0) || (!compileFlag && compilePathArg != -1)) {
        C_E; wprintf(L"  Usage: bmg [-cli | -config \"path\" | -edit \"path\" | -compile \"path\"]\n"); C_R;
        return 2;
    }

    if (compileFlag) {
        if (compilePathArg == -1) {
            C_E; wprintf(L"  Usage: bmg -compile \"path\"\n"); C_R;
            return 2;
        }
        std::wstring target = argv[compilePathArg];
        if (target.size() > 2 && target.substr(0, 2) == L"--") {
            std::wstring stripped = target.substr(2);
            if (GetFileAttributesW(target.c_str()) == INVALID_FILE_ATTRIBUTES &&
                GetFileAttributesW(stripped.c_str()) != INVALID_FILE_ATTRIBUTES) {
                target = stripped;
            }
        }

        if (GetFileAttributesW(target.c_str()) == INVALID_FILE_ATTRIBUTES) {
            C_E; wprintf(L"  [Error] Target batch file not found: %ls\n", target.c_str()); C_R;
            return 2;
        }

        std::wstring outputExe = GetPathDirectory(target) + L"\\"
            + GetPathBaseName(target) + L".exe";

        C_H; wprintf(L"  BatchMenuGenerator EXE compiler\n"); C_R;
        if (CompileBatchToExe(target, outputExe)) return 0;
        return 1;
    }

    if (!skipConsoleRelaunch && IsHostedInModernTerminal() && RelaunchInClassicConsole(argc, argv))
        return 0;

    if (!DirectoryExists(GetBatchMenuGeneratorAppDataDir(false))) {
        SetConsoleTitleW(L"First time Setup");
        system("cls");
        if (!g_enableTUI) {
            C_H; wprintf(L"  First time Setup\n\n"); C_R;
        }
        C_T; wprintf(L"  It looks like you're using the program for the first time.\n");
        wprintf(L"  Hi! For the best experience, we recommend that you complete this quick setup.\n");
        wprintf(L"  It is completely optional and you can find it again at any time in\n");
        wprintf(L"  Settings > Set Up program.\n\n"); C_R;

        int setupChoice = 0;
        if (g_enableTUI) {
            const int selected = RunTUIMenu(L"First time Setup",
                {L"Yes, continue to setup", L"No, continue to main menu"}, 1, -1, false);
            setupChoice = selected < 0 ? 2 : selected + 1;
        }
        while (setupChoice != 1 && setupChoice != 2) {
            C_P; wprintf(L"    1) Yes, continue to setup\n");
            wprintf(L"    2) No, continue to main menu\n\n  Select option [2]: "); C_I;
            std::wstring choice = ReadLine(); C_R;
            if (choice.empty()) choice = L"2";
            if (choice == L"1") setupChoice = 1;
            else if (choice == L"2") setupChoice = 2;
            else { C_E; wprintf(L"  Please enter 1 or 2.\n\n"); C_R; }
        }

        if (setupChoice == 1) RunProgramSetupMenu();
        SetConsoleTitleW(L"BatchMenuGenerator");
        if (g_exitAfterMove) return 0;
    }

    std::wstring startupGeneratedEditPath;
    if (startupMode == 3) {
        std::vector<std::wstring> editLines;
        if (!ReadConsoleEditorTextFile(startupPath, editLines)) {
            C_E; wprintf(L"  [Error] Could not open text file: %ls\n", startupPath.c_str()); C_R;
            return 2;
        }
        if (!IsBatchMenuGeneratorScript(editLines)) {
            if (!ConsoleTextEditor(editLines, startupPath)) return 0;
            if (!SaveConsoleEditorTextToPath(editLines, startupPath)) {
                C_E; wprintf(L"  [Error] Could not save text file: %ls\n", startupPath.c_str()); C_R;
                return 1;
            }
            return 0;
        }
        startupGeneratedEditPath = startupPath;
    }

    std::vector<std::wstring> configWarnings;
    if (g_appConfig.bconfig == 1) {
        LoadSettingsFile(g_appConfig, configWarnings);
    }

    std::wstring autoSavePath = GetAutoSavePath();
    std::wstring autoSaveTimestamp = ReadAutoSaveTimestamp(autoSavePath);
    std::vector<std::wstring> autosaveData = ReadAutoSave(autoSavePath);
    bool hasAutosave = !autosaveData.empty() && startupMode == 0;

    while (true) {
        if (!g_debugMode) system("cls");

        C_B; wprintf(L"\n");
        TuiBannerBorder();
        TuiBannerTitle(L"BatchMenuGenerator");
        if (g_debugMode) { C_E; TuiBannerTitle(L"** DEBUG MODE **"); }
        C_B; TuiBannerBorder();
        wprintf(L"\n"); C_R;

        if (!configWarnings.empty()) {
            C_E; wprintf(L"  [Settings Warnings]\n");
            for (const auto& warn : configWarnings) {
                wprintf(L"   * %ls\n", warn.c_str());
            }
            wprintf(L"\n"); C_R;
            configWarnings.clear();
        }

        std::wstring workflowChoice;
        std::wstring title;
        std::vector<std::wstring> description;
        std::wstring password = L"";
        std::vector<OptionData> options;
        std::wstring autoFilename = L"", autoPath = L"";
        bool silentPreviewOnly = false, requireAdmin = false;
        std::wstring finalSavedPath = L"", tempFilePath = L"";
        wchar_t borderChar = L'=';
        std::wstring resumeStep;
        bool resumingAutosave = false;
        bool wizardSession = false;

        if (hasAutosave) {
            C_N; wprintf(L"  [Autosave found from %ls]\n", autoSaveTimestamp.c_str());

            wprintf(L"  Would you like to continue generating the batch script? (Y/n): "); C_I;
            std::wstring asAns = ReadLine(); C_R;
            bool resumeAs = (asAns.empty() || asAns == L"Y" || asAns == L"y" || asAns == L"yes");

            if (resumeAs) {
                std::wstring savedAutoFilename, savedAutoPath;
                bool savedSilentPreview = false;
                std::wstring savedStep = L"";

                for (const auto& line : autosaveData) {
                    std::wstring wl = Trim(line);
                    if (wl.substr(0, 5) == L"step ") savedStep = Trim(wl.substr(5));
                }

                if (ParseScriptConfigLines(autosaveData, title, description, password, options, savedAutoFilename, savedAutoPath, savedSilentPreview, requireAdmin)) {
                    C_S; wprintf(L"  [Autosave restored - resuming at step: %ls]\n", savedStep.c_str()); C_R;
                    Sleep(1000);
                    resumingAutosave = true;
                    resumeStep = savedStep;
                } else { C_E; wprintf(L"  [Autosave data corrupted, starting fresh]\n"); C_R; }
            }

            hasAutosave = false;
            if (!resumingAutosave) {
                DeleteAutoSave(autoSavePath);
                system("cls"); continue;
            }
        }

        if (resumingAutosave) {
            workflowChoice = L"1";
        } else if (startupMode == 1 || startupMode == 2) {
            workflowChoice = L"2";
        } else if (startupMode == 3 && !startupGeneratedEditPath.empty()) {
            workflowChoice = L"3";
        } else if (g_enableTUI) {
            std::vector<std::wstring> tuiOptions;
            std::vector<std::wstring> optionKeys;

            tuiOptions.push_back(L"Make new Batch Menu"); optionKeys.push_back(L"1");
            tuiOptions.push_back(L"Text Mode"); optionKeys.push_back(L"2");
            tuiOptions.push_back(L"Edit existing generated .bat script"); optionKeys.push_back(L"3");

            if (g_appConfig.bconfig == 1) {
                tuiOptions.push_back(L"Settings (bconfig)"); optionKeys.push_back(L"S");
            }
            tuiOptions.push_back(L"Exit Program"); optionKeys.push_back(L"0");

            int chosenIndex = RunTUIMenu(L"BatchMenuGenerator", tuiOptions);

            if (chosenIndex == -1 || optionKeys[chosenIndex] == L"0") {
                break;
            }

            workflowChoice = optionKeys[chosenIndex];
        } else {
            C_P; wprintf(L"  Select Mode:\n    1 = Make new Batch Menu\n    2 = Text Mode\n    3 = Edit existing generated .bat script\n");
            if (g_appConfig.bconfig == 1) wprintf(L"    S = Settings (bconfig)\n");

            wprintf(L"    0 = Exit Program\n  Choice [1]: "); C_I;
            workflowChoice = ReadLine(); C_R;
            if (workflowChoice.empty()) workflowChoice = L"1";
            if (workflowChoice == L"0") break;
        }

        if (workflowChoice == L"S" || workflowChoice == L"s") {
            if (g_appConfig.bconfig == 1) {
                RunSettingsMenu();
                if (g_exitAfterMove) return 0;
                configWarnings.clear();
                ReloadSettingsFromDisk(configWarnings);
                continue;
            }
        }

        if (workflowChoice == L"3") {
            if (!OpenGeneratedBatchEditor(title, description, password, options, requireAdmin, borderChar,
                finalSavedPath, startupGeneratedEditPath)) {
                if (!startupGeneratedEditPath.empty()) return 1;
                continue;
            }
            startupGeneratedEditPath.clear();
            startupMode = 0;

            int nextChoice = -1;
            if (g_appConfig.bconfig == 1 && g_appConfig.end > 0) {
                nextChoice = g_appConfig.end - 1;
            } else if (g_enableTUI) {
                nextChoice = RunTUIMenu(L"What would you like to do next?",
                    { L"Main menu", L"Launch script", L"Exit" }, 2, -1, false);
                if (nextChoice < 0) nextChoice = 2;
            } else {
                while (true) {
                    C_P; wprintf(L"\n  ============================================\n");
                    wprintf(L"  What would you like to do next?\n    1) Main menu\n    2) Launch script\n    3) Exit\n  Select [3]: "); C_I;
                    std::wstring endChoice = ReadLine(); C_R;
                    if (endChoice.empty()) endChoice = L"3";
                    if (endChoice == L"1" || endChoice == L"2" || endChoice == L"3") {
                        nextChoice = (int)_wtoi(endChoice.c_str()) - 1;
                        break;
                    }
                }
            }

            if (nextChoice == 2) break;
            if (nextChoice == 1) {
                ShellExecuteW(NULL, L"open", finalSavedPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                Sleep(3000);
            }
            continue;
        }

        if (workflowChoice == L"2211") { g_debugMode = !g_debugMode; if (g_debugMode) RunDebugMenu(); continue; }

        if (workflowChoice == L"2") {
            int txtChoice = -1;
            while (true) {
                if (startupMode == 2) {
                    if (!ParseScriptConfig(startupPath, title, description, password, options,
                        autoFilename, autoPath, silentPreviewOnly, requireAdmin, &borderChar)) {
                        C_E; wprintf(L"  [Error] Failed to load config file: %ls\n", startupPath.c_str()); C_R;
                        return 2;
                    }
                    startupMode = 0;
                    txtChoice = 6;
                    break;
                } else if (startupMode == 1) {
                    startupMode = 0;
                    txtChoice = 1;
                } else if (g_enableTUI) {
                    txtChoice = RunTUIMenu(L"Text Mode", {
                        L"Edit in Notepad",
                        L"Edit in CLI",
                        L"Load TXT config",
                        L"Show syntax tutorial",
                        L"Exit to Main Menu",
                        L"Exit Program & Save"
                    });
                } else {
                    system("cls");
                    C_P; wprintf(L"\n  Text Mode:\n    1) Edit in Notepad\n    2) Edit in CLI\n    3) Load TXT config\n    4) Show syntax tutorial\n    5) Exit to Main Menu\n    X) Exit Program & Save\n  Select: "); C_I;
                    std::wstring a = ReadLine(); C_R;
                    if (a == L"1") txtChoice = 0;
                    else if (a == L"2") txtChoice = 1;
                    else if (a == L"3") txtChoice = 2;
                    else if (a == L"4") txtChoice = 3;
                    else if (a == L"5") txtChoice = 4;
                    else if (a == L"X" || a == L"x") txtChoice = 5;
                    else txtChoice = -1;
                }

                if (txtChoice == 4 || txtChoice == -1) break;
                if (txtChoice == 5) exit(0);

                if (txtChoice == 0) {
                    std::wstring tmp = GetAppDataDir() + L"bmg_notepad_edit.txt";
                    if (GetFileAttributesW(tmp.c_str()) == INVALID_FILE_ATTRIBUTES) {
                        std::ofstream out(tmp);
                        out << "";
                        out.close();
                    }
                    OpenInNotepadAndWait(tmp);

                    if (ParseScriptConfig(tmp, title, description, password, options, autoFilename, autoPath, silentPreviewOnly, requireAdmin, &borderChar)) {
                        break;
                    } else {
                        C_E; wprintf(L"  [Error] Failed to parse notepad file.\n"); C_R;
                        system("pause");
                    }
                } else if (txtChoice == 1) {
                    std::vector<std::wstring> editorLines;
                    bool compile = ConsoleTextEditor(editorLines);
                    if (compile && ParseScriptConfigLines(editorLines, title, description, password, options, autoFilename, autoPath, silentPreviewOnly, requireAdmin, &borderChar)) {
                        break;
                    } else if (compile) {
                        C_E; wprintf(L"  [Error] Failed to process editor text!\n"); C_R;
                        system("pause");
                    }
                } else if (txtChoice == 2) {
                    bool loaded = false; std::wstring loadChoice = L"";
                    if (g_appConfig.bconfig == 1 && g_appConfig.saveload > 0) {
                        loadChoice = (g_appConfig.saveload == 1) ? L"1" : L"2";
                    }
                    while (!loaded) {
                        if (loadChoice.empty()) {
                            if (g_enableTUI) {
                                int lc = RunTUIMenu(L"Choose Load Method", {L"Browse (Windows Explorer)", L"Type path manually", L"Cancel"});
                                if (lc == 2 || lc == -1) break;
                                loadChoice = (lc == 0) ? L"1" : L"2";
                            } else {
                                C_P; wprintf(L"\n  Choose Load Method:\n    1 = Browse\n    2 = Type path\n    3 = Cancel\n  Select [1]: "); C_I;
                                loadChoice = ReadLine(); C_R;
                                if (loadChoice == L"3") break;
                                if (loadChoice.empty()) loadChoice = L"1";
                            }
                        }
                        if (loadChoice != L"1" && loadChoice != L"2") { loadChoice = L""; continue; }

                        std::wstring configPath = L"";
                        if (loadChoice == L"1") {
                            wchar_t path[MAX_PATH] = {0}; OPENFILENAMEW of; ZeroMemory(&of, sizeof(of));
                            of.lStructSize = sizeof(of); of.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
                            of.lpstrFile = path; of.nMaxFile = MAX_PATH; of.lpstrDefExt = L"txt";
                            of.lpstrTitle = L"Select Configuration File"; of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                            if (GetOpenFileNameW(&of)) configPath = path;
                            else { C_P; wprintf(L"\n  Dialog canceled.\n"); C_R; loadChoice = L"2"; continue; }
                        } else { C_P; wprintf(L"\n  Enter config file path: "); C_I; configPath = ReadLine(); C_R; }

                        if (configPath.length() >= 2 && configPath.front() == L'"' && configPath.back() == L'"')
                            configPath = configPath.substr(1, configPath.length() - 2);
                        if (configPath.empty() || !ParseScriptConfig(configPath, title, description, password, options, autoFilename, autoPath, silentPreviewOnly, requireAdmin, &borderChar)) {
                            C_E; wprintf(L"  [Error] Failed to load config file.\n"); C_R;
                            if (g_enableTUI) {
                                int r = RunTUIMenu(L"Error loading file", {L"Try again", L"Cancel"});
                                if (r == 1 || r == -1) break;
                            } else {
                                C_P; wprintf(L"    1) Try again\n    2) Cancel\n  Select [1]: "); C_I;
                                std::wstring retryAns = ReadLine(); C_R;
                                if (retryAns == L"2") break;
                            }
                            loadChoice = L"";
                        } else loaded = true;
                    }
                    if (loaded) break;
                } else if (txtChoice == 3) {
                    ShowSyntaxTutorial();
                }
            }
            if (txtChoice == 4 || txtChoice == -1) { system("cls"); continue; }
        } else {
            if (!RunMode1Flow(title, description, password, options, requireAdmin, borderChar, resumeStep)) {
                system("cls"); continue;
            }
            wizardSession = true;
        }

        if (workflowChoice == L"2") {
        } else if (workflowChoice == L"3") {
        } else if (g_appConfig.bconfig == 1 && g_appConfig.menuborder > 0) {
            if (g_appConfig.menuborder == 1) borderChar = L'=';
            else if (g_appConfig.menuborder == 2) borderChar = L'-';
            else if (g_appConfig.menuborder == 3) borderChar = L'Z';
        } else if (g_enableTUI) {
            std::vector<std::wstring> bOpts = {
                L"Classic  :  =========================================================",
                L"Modern   :  ---------------------------------------------------------",
                L"Zigzag   :  ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ",
                L"Custom   :  (Enter custom character)",
                L"Exit & Save"
            };
            int bIdx = RunTUIMenu(L"Select Border Style", bOpts, 0, -1, false);
            if (bIdx == 4 || bIdx == -1) exit(0);
            if (bIdx == 1) borderChar = L'-';
            else if (bIdx == 2) borderChar = L'Z';
            else if (bIdx == 3) {
                C_P; wprintf(L"  Enter border character: "); C_I; std::wstring custBorder = ReadLine(); C_R;
                if (!custBorder.empty()) borderChar = custBorder[0];
            } else borderChar = L'=';
        } else {
            C_P; wprintf(L"\n  Select border style:\n    1) Classic  :  =========================================================\n");
            wprintf(L"    2) Modern   :  ---------------------------------------------------------\n");
            wprintf(L"    3) Zigzag   :  ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ\n");
            wprintf(L"    4) Custom   :  (Enter any character you want)\n    X) Exit & Save\n  Select (1-4) [1]: "); C_I;
            std::wstring borderChoice = ReadLine(); C_R;
            if (borderChoice == L"X" || borderChoice == L"x") exit(0);
            if (borderChoice == L"2") borderChar = L'-';
            else if (borderChoice == L"3") borderChar = L'Z';
            else if (borderChoice == L"4") { C_P; wprintf(L"  Enter border character: "); C_I; std::wstring custBorder = ReadLine(); C_R; if (!custBorder.empty()) borderChar = custBorder[0]; }
        }

        C_H; wprintf(L"\n  Generating batch script...\n"); C_R;
        std::wstring script = GenerateBatch(title, description, options, password, requireAdmin, borderChar);
        C_B; wprintf(L"\n  ========== PREVIEW ==========\n"); C_R;
        wprintf(L"%ls\n", script.c_str());

        if (workflowChoice == L"2" && silentPreviewOnly) {
            C_P; wprintf(L"\n  Not saved! Exiting preview mode.\n"); C_R;
        } else if (workflowChoice == L"2" && !autoFilename.empty()) {
            if (autoFilename.size() >= 4 && _wcsicmp(autoFilename.c_str() + autoFilename.size() - 4, L".exe") == 0) {
                SaveConfigScriptAsExe(script, autoFilename, autoPath, requireAdmin);
            } else {
                SaveToManualPath(script, finalSavedPath, autoFilename, autoPath, !requireAdmin);
            }
        } else {
            bool saveAnsBool = false;
            if (g_enableTUI) {
                int saveChoice = RunTUIMenu(L"Save to file?", {L"Yes, save the generated batch", L"No, continue without saving", L"Exit"}, 0, -1, false);
                if (saveChoice == -1 || saveChoice == 2) exit(0);
                saveAnsBool = (saveChoice == 0);
            } else {
                C_P; wprintf(L"  Save to file? (Y/n) [X to exit]: "); C_I;
                std::wstring saveAns = ReadLine(); C_R;
                if (saveAns == L"X" || saveAns == L"x") exit(0);
                saveAnsBool = (saveAns.empty() || saveAns == L"Y" || saveAns == L"y" || saveAns == L"yes");
            }

            if (saveAnsBool) {
                std::wstring methodChoice;
                if (g_appConfig.bconfig == 1 && g_appConfig.saveload > 0) {
                    methodChoice = (g_appConfig.saveload == 1) ? L"2" : L"1";
                } else {
                    if (g_enableTUI) {
                        int saveMethod = RunTUIMenu(L"Choose Save Method", {L"Type path manually", L"Browse (Windows File Explorer dialog)", L"Exit"}, 1, -1, false);
                        if (saveMethod == -1 || saveMethod == 2) exit(0);
                        methodChoice = (saveMethod == 0) ? L"1" : L"2";
                    } else {
                        while (true) {
                            C_P; wprintf(L"  Choose Save Method:\n    1 = Type path manually\n    2 = Browse (Windows File Explorer dialog)\n    X = Exit & Save\n  Select [2]: "); C_I;
                            methodChoice = ReadLine(); C_R;
                            if (methodChoice == L"X" || methodChoice == L"x") exit(0);
                            if (methodChoice.empty()) methodChoice = L"2";
                            if (methodChoice == L"1" || methodChoice == L"2") break;
                            C_E; wprintf(L"  Invalid option choice.\n"); C_R;
                        }
                    }
                }

                if (methodChoice == L"1") SaveToManualPath(script, finalSavedPath, L"", L"", !requireAdmin);
                else {
                    wchar_t path[MAX_PATH] = {0}; OPENFILENAMEW of; ZeroMemory(&of, sizeof(of));
                    of.lStructSize = sizeof(of); of.lpstrFilter = L"Batch Files (*.bat)\0*.bat\0All Files (*.*)\0*.*\0\0";
                    of.lpstrFile = path; of.nMaxFile = MAX_PATH; of.lpstrDefExt = L"bat";
                    of.lpstrTitle = L"Save Batch Menu Script"; of.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
                    if (GetSaveFileNameW(&of)) {
                        if (!WriteScriptToFile(path, script, !requireAdmin)) { C_E; wprintf(L"\n  [ERROR] Explorer save failed.\n"); C_R; SaveToManualPath(script, finalSavedPath, L"", L"", !requireAdmin); }
                        else { C_S; wprintf(L"\n  [SUCCESS] Saved to: %ls\n", path); C_R; finalSavedPath = path; }
                    } else { C_P; wprintf(L"\n  Dialog canceled. Defaulting to manual path entry...\n"); C_R; SaveToManualPath(script, finalSavedPath, L"", L"", !requireAdmin); }
                }
            }

            if (wizardSession) DeleteAutoSave(autoSavePath);

            if (!silentPreviewOnly) {
                bool compileExe = false;
                if (g_enableTUI) {
                    int compileChoice = RunTUIMenu(L"Compile this batch into an EXE?", {L"Yes, compile to EXE", L"No, continue"}, 1, -1, false);
                    compileExe = (compileChoice == 0);
                } else {
                    C_P; wprintf(L"\n  Compile this batch into an EXE? (y/N): "); C_I;
                    std::wstring compileAns = ReadLine(); C_R;
                    compileExe = (compileAns == L"y" || compileAns == L"Y" || compileAns == L"yes");
                }
                if (compileExe) CompileGeneratedScriptToExeInteractive(script, finalSavedPath);
            }

            std::wstring endChoice;
            if (g_appConfig.bconfig == 1 && g_appConfig.end > 0) {
                endChoice = std::to_wstring(g_appConfig.end);
            } else if (g_enableTUI) {
                int nextChoice = RunTUIMenu(L"What would you like to do next?", {L"Main menu", L"Launch script", L"Exit"}, 2, -1, false);
                if (nextChoice < 0) nextChoice = 2;
                endChoice = std::to_wstring(nextChoice + 1);
            } else {
                while (true) {
                    C_P; wprintf(L"\n  ============================================\n");
                    wprintf(L"  What would you like to do next?\n    1) Main menu\n    2) Launch script\n    3) Exit\n  Select [3]: "); C_I;
                    endChoice = ReadLine(); C_R;
                    if (endChoice.empty()) endChoice = L"3";
                    if (endChoice == L"1" || endChoice == L"2" || endChoice == L"3") break;
                }
            }

            if (endChoice == L"3") { DeleteTempFile(tempFilePath); break; }
            else if (endChoice == L"2") {
                if (finalSavedPath.empty()) {
                    tempFilePath = GenerateTempFilePath();
                    if (!WriteScriptToFile(tempFilePath, script, !requireAdmin)) { C_E; wprintf(L"  [Error] Could not create temp file!\n"); C_R; continue; }
                    finalSavedPath = tempFilePath;
                }
                ShellExecuteW(NULL, L"open", finalSavedPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                Sleep(3000); DeleteTempFile(tempFilePath); tempFilePath = L"";
            }
        }
    }
    return 0;
}
