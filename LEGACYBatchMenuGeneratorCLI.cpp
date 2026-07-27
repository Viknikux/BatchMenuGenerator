#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <commdlg.h> 
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <io.h>
#include <fcntl.h>

#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")

// ---------------------------------------------------------------------------
// Console color configuration
// ---------------------------------------------------------------------------
enum ConsoleColor {
    CC_RESET   = 7,
    CC_CYAN    = 11,
    CC_YELLOW  = 14,
    CC_GREEN   = 10,
    CC_RED     = 12,
    CC_WHITE   = 15,
    CC_MAGENTA = 13
};

static void SetConsoleColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
}

static std::wstring ReadLine() {
    std::wstring line;
    std::getline(std::wcin, line);
    return line;
}

// ---------------------------------------------------------------------------
// Batch Escape
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Generate the batch script
// ---------------------------------------------------------------------------
struct OptionData {
    std::wstring label;
    std::vector<std::wstring> commands; 
    int runMode; 
};

static std::wstring GenerateBatch(const std::wstring& title,
    const std::vector<std::wstring>& description,
    const std::vector<OptionData>& options) {

    std::wstring bat;
    bat.reserve(8192);
    bat+=L"Created with BatchMenuGenerator!\r\n";
    bat += L"@echo off\r\ntitle " + title + L"\r\n:menu\r\ncls\r\n";
    
    bat += L"echo =========================================================\r\n";
    
    const int BANNER_WIDTH = 57;
    int titleLen = (int)title.length();
    int padLeft = (BANNER_WIDTH - titleLen) / 2;
    if (padLeft < 0) padLeft = 0;
    
    std::wstring spaces(padLeft, L' ');
    bat += L"echo " + spaces + title + L"\r\n";
    bat += L"echo =========================================================\r\necho.\r\n";

    if (!description.empty()) {
        for (const auto& line : description) {
            if (line.empty()) {
                bat += L"echo.\r\n";
            } else {
                bat += L"echo  " + EscapeBatch(line, false) + L"\r\n";
            }
        }
        bat += L"echo.\r\n";
        bat += L"echo =========================================================\r\necho.\r\n";
    }

    for (size_t i = 0; i < options.size(); i++) {
        std::wstring l = options[i].label.empty() ? L"Option" : options[i].label;
        wchar_t b[256];
        swprintf(b, 256, L"echo  %zu) %s\r\n", i + 1, l.c_str());
        bat += b;
    }
    bat += L"echo  X) Exit\r\necho.\r\n";
    bat += L"echo =========================================================\r\necho.\r\n";

    wchar_t b[256];
    swprintf(b, 256, L"set /p choice=\"Select Option (1-%zu): \"\r\n", options.size());
    bat += b;

    for (size_t i = 0; i < options.size(); i++) {
        swprintf(b, 256, L"if \"%%choice%%\"==\"%zu\" goto run_option%zu\r\n", i + 1, i + 1);
        bat += b;
    }
    bat += L"if \"%choice%\"==\"X\" goto close_menu\r\n";
    bat += L"if \"%choice%\"==\"x\" goto close_menu\r\ngoto menu\r\n\r\n";

    for (size_t i = 0; i < options.size(); i++) {
        swprintf(b, 256, L":run_option%zu\r\n", i + 1);
        bat += b;
        bat += L"cls\r\n";

        if (options[i].commands.empty()) {
            bat += L"echo No command specified.\r\n";
        } else {
            for (size_t j = 0; j < options[i].commands.size(); j++) {
                std::wstring cmd = options[i].commands[j];
                if (cmd.length() >= 2 && cmd[0] == L'"' && cmd.back() == L'"')
                    cmd = cmd.substr(1, cmd.length() - 2);

                if (cmd.empty()) continue;

                if (options[i].runMode == 1) {
                    bat += L"cmd /c \"" + EscapeBatch(cmd, true) + L"\"\r\n";
                } else {
                    bat += L"call " + EscapeBatch(cmd, false) + L"\r\n";
                }
            }
        }

        bat += L"echo.\r\npause\r\ngoto menu\r\n\r\n";
    }

    bat += L":close_menu\r\ncls\r\ntitle cmd\r\ngoto :eof\r\n";
    return bat;
}

// ---------------------------------------------------------------------------
// File Handling
// ---------------------------------------------------------------------------
static bool WriteScriptToFile(const std::wstring& path, const std::wstring& script) {
    HANDLE hf = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return false;

    DWORD wr;
    BYTE bom[] = { 0xEF, 0xBB, 0xBF };
    WriteFile(hf, bom, 3, &wr, NULL);
    int len = WideCharToMultiByte(CP_UTF8, 0, script.c_str(),
        (int)script.length(), NULL, 0, NULL, NULL);
    if (len > 0) {
        std::vector<char> u(len);
        WideCharToMultiByte(CP_UTF8, 0, script.c_str(),
            (int)script.length(), u.data(), len, NULL, NULL);
        WriteFile(hf, u.data(), len, &wr, NULL);
    }
    CloseHandle(hf);
    return true;
}

static bool SaveToManualPath(const std::wstring& script) {
    while (true) {
        SetConsoleColor(CC_YELLOW);
        wprintf(L"\n  Enter filename: ");
        SetConsoleColor(CC_RESET);
        std::wstring filename = ReadLine();
        if (filename.empty()) {
            SetConsoleColor(CC_RED);
            wprintf(L"  [Error] Filename cannot be empty.\n");
            SetConsoleColor(CC_RESET);
            continue;
        }

        if (filename.length() < 4 || _wcsicmp(filename.c_str() + filename.length() - 4, L".bat") != 0) {
            filename += L".bat";
        }

        SetConsoleColor(CC_YELLOW);
        wprintf(L"  Enter path [Press Enter for Current (program) Directory]: ");
        SetConsoleColor(CC_RESET);
        std::wstring dir = ReadLine();

        if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/') {
            dir += L"\\";
        }

        std::wstring fullPath = dir + filename;
        std::wstring baseName = filename.substr(0, filename.length() - 4);

        std::wstring checkDir = dir;
        if (!checkDir.empty() && (checkDir.back() == L'\\' || checkDir.back() == L'/') && checkDir.length() > 3) {
            checkDir.pop_back(); 
        }

        if (!checkDir.empty()) {
            DWORD attr = GetFileAttributesW(checkDir.c_str());
            if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                SetConsoleColor(CC_RED);
                wprintf(L"  [Error] Destination folder does not exist!\n");
                SetConsoleColor(CC_YELLOW);
                wprintf(L"    1) Retype file details\n");
                wprintf(L"    2) Create folder automatically\n");
                wprintf(L"  Select option (1-2) [1]: ");
                SetConsoleColor(CC_RESET);
                std::wstring opt = ReadLine();
                
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
                        SetConsoleColor(CC_RED);
                        wprintf(L"  [Error] Failed to create directory. (probably due to lack of permissions in destination folder)\n");
                        SetConsoleColor(CC_RESET);
                        continue;
                    }
                } else {
                    continue;
                }
            }
        }

        DWORD fileAttr = GetFileAttributesW(fullPath.c_str());
        if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
            SetConsoleColor(CC_RED);
            wprintf(L"  [Warning] A file with the same name already exists!\n");
            SetConsoleColor(CC_YELLOW);
            wprintf(L"    1) Replace file\n");
            wprintf(L"    2) Rename file to %s1.bat\n", baseName.c_str());
            wprintf(L"    3) Retype file details\n");
            wprintf(L"  Select option (1-3) [1]: ");
            SetConsoleColor(CC_RESET);
            std::wstring opt = ReadLine();
            
            if (opt == L"2") {
                fullPath = dir + baseName + L"1.bat";
            } else if (opt == L"3") {
                continue;
            }
        }

        if (WriteScriptToFile(fullPath, script)) {
            SetConsoleColor(CC_GREEN);
            wprintf(L"  [SUCCESS] Saved to: %s\n", fullPath.c_str());
            SetConsoleColor(CC_RESET);
            return true;
        } else {
            SetConsoleColor(CC_RED);
            wprintf(L"  [ERROR] Write operations restricted at this path (probably due to lack of permissions).\n");
            SetConsoleColor(CC_RESET);
        }
    }
}

// ---------------------------------------------------------------------------
// Main Menu
// ---------------------------------------------------------------------------
int main() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    SetConsoleColor(CC_CYAN);
    wprintf(L"\n");
    wprintf(L"  ============================================\n");
    wprintf(L"       Batch Menu Generator - CLI Edition\n");
    wprintf(L"  ============================================\n\n");
    SetConsoleColor(CC_RESET);

    std::wstring title;
    while (true) {
        SetConsoleColor(CC_YELLOW);
        wprintf(L"  Enter window title (Max 57 chars): ");
        SetConsoleColor(CC_RESET);
        title = ReadLine();
        if (title.empty()) {
            title = L"Batch Menu";
            break;
        }
        if (title.length() <= 57) {
            break;
        } else {
            SetConsoleColor(CC_RED);
            wprintf(L"  Title invalid: cannot be longer than 57 characters (including spaces)!\n");
            SetConsoleColor(CC_RESET);
        }
    }

    std::vector<std::wstring> description;
    SetConsoleColor(CC_YELLOW);
    wprintf(L"  Do you want to add a description? (Y/n): ");
    SetConsoleColor(CC_RESET);
    std::wstring descAns = ReadLine();

    if (descAns.empty() || descAns == L"Y" || descAns == L"y" || descAns == L"yes") {
        SetConsoleColor(CC_YELLOW);
        wprintf(L"  Enter description lines below.\n");
        wprintf(L"  Press Enter on an empty line when done.\n");
        SetConsoleColor(CC_GREEN);
        wprintf(L"  Description:\n");
        SetConsoleColor(CC_RESET);

        while (true) {
            wprintf(L"    > ");
            std::wstring descLine = ReadLine();
            if (descLine.empty()) break;
            description.push_back(descLine);
        }
    }

    SetConsoleColor(CC_YELLOW);
    wprintf(L"  Number of options (1-20) [3]: ");
    SetConsoleColor(CC_RESET);
    std::wstring numStr = ReadLine();
    int optionCount = 3;
    if (!numStr.empty()) {
        wchar_t* end = nullptr;
        int val = (int)wcstol(numStr.c_str(), &end, 10);
        if (end && *end == L'\0' && val >= 1 && val <= 20)
            optionCount = val;
    }

    std::vector<OptionData> options(optionCount);

    for (int i = 0; i < optionCount; i++) {
        SetConsoleColor(CC_MAGENTA);
        wprintf(L"\n  --- Option %d ---\n", i + 1);
        SetConsoleColor(CC_RESET);

        SetConsoleColor(CC_YELLOW);
        wprintf(L"  Label [Option %d]: ", i + 1);
        SetConsoleColor(CC_RESET);
        std::wstring label = ReadLine();
        options[i].label = label.empty() ? L"Option" : label;

        SetConsoleColor(CC_YELLOW);
        wprintf(L"  Run mode:\n");
        wprintf(L"    0 = Direct call (%%command%%)\n");
        wprintf(L"    1 = cmd /c wrapper\n");
        wprintf(L"  Select (0/1) [0]: ");
        SetConsoleColor(CC_RESET);
        std::wstring modeStr = ReadLine();
        options[i].runMode = (modeStr == L"1") ? 1 : 0;

        SetConsoleColor(CC_YELLOW);
        wprintf(L"  Enter command(s), one per line.\n");
        wprintf(L"  Press Enter on an empty line when done.\n");
        SetConsoleColor(CC_GREEN);
        wprintf(L"  Commands:\n");
        SetConsoleColor(CC_RESET);

        while (true) {
            wprintf(L"    > ");
            std::wstring cmd = ReadLine();
            if (cmd.empty()) break;
            options[i].commands.push_back(cmd);
        }

        if (options[i].commands.empty()) {
            SetConsoleColor(CC_RED);
            wprintf(L"  [No commands entered for this option!]\n");
            SetConsoleColor(CC_RESET);
        } else {
            SetConsoleColor(CC_GREEN);
            wprintf(L"  [%zu command(s) recorded]\n", options[i].commands.size());
            SetConsoleColor(CC_RESET);
        }
    }

    SetConsoleColor(CC_CYAN);
    wprintf(L"\n  Generating batch script...\n");
    SetConsoleColor(CC_RESET);

    std::wstring script = GenerateBatch(title, description, options);

    SetConsoleColor(CC_WHITE);
    wprintf(L"\n  ========== PREVIEW ==========\n");
    SetConsoleColor(CC_RESET);
    wprintf(L"%s\n", script.c_str());

    SetConsoleColor(CC_YELLOW);
    wprintf(L"  Save to file? (Y/n): ");
    SetConsoleColor(CC_RESET);
    std::wstring saveAns = ReadLine();

    if (saveAns.empty() || saveAns == L"Y" || saveAns == L"y" || saveAns == L"yes") {
        std::wstring methodChoice;
        
        while (true) {
            SetConsoleColor(CC_YELLOW);
            wprintf(L"  Choose Save Method:\n");
            wprintf(L"    1 = Type path manually (CLI)\n");
            wprintf(L"    2 = Browse (Windows File Explorer dialog)\n");
            wprintf(L"  Select (1/2) [2]: ");
            SetConsoleColor(CC_RESET);
            methodChoice = ReadLine();
            
            if (methodChoice.empty()) methodChoice = L"2";
            if (methodChoice == L"1" || methodChoice == L"2") break;

            SetConsoleColor(CC_RED);
            wprintf(L"  Invalid option choice. Select 1 or 2.\n");
            SetConsoleColor(CC_RESET);
        }

        if (methodChoice == L"1") {
            SaveToManualPath(script);
        } else {
            wchar_t path[MAX_PATH] = { 0 };
            OPENFILENAMEW of = { sizeof(of) };
            of.lpstrFilter = L"Batch Files\0*.bat\0All Files\0*.*\0";
            of.lpstrFile = path;
            of.nMaxFile = MAX_PATH;
            of.lpstrDefExt = L"bat";
            of.lpstrTitle = L"Save Batch Menu Script";
            of.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

            if (GetSaveFileNameW(&of)) {
                if (!WriteScriptToFile(path, script)) {
                    SetConsoleColor(CC_RED);
                    wprintf(L"\n  [ERROR] Explorer save failed.\n");
                    SetConsoleColor(CC_RESET);
                    SaveToManualPath(script);
                } else {
                    SetConsoleColor(CC_GREEN);
                    wprintf(L"\n  [SUCCESS] Saved to: %s\n", path);
                    SetConsoleColor(CC_RESET);
                }
            } else {
                SetConsoleColor(CC_YELLOW);
                wprintf(L"\n  Explorer engine unavailable or aborted. Defaulting to manual path selection...\n");
                SetConsoleColor(CC_RESET);
                SaveToManualPath(script);
            }
        }
    } else {
        SetConsoleColor(CC_YELLOW);
        wprintf(L"\n  Not saved! Exiting.\n");
        SetConsoleColor(CC_RESET);
    }

    SetConsoleColor(CC_CYAN);
    wprintf(L"\n  Press Enter to exit...");
    SetConsoleColor(CC_RESET);
    ReadLine();
    return 0;
}