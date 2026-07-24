//Created with love, and electricity
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#include <commdlg.h> 
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")

// ============================================================
// Theme configuration
// ============================================================
enum ConsoleColor {
    CC_RESET       = 7,
    CC_CYAN        = 11,
    CC_YELLOW      = 14,
    CC_GREEN       = 10,
    CC_RED         = 12,
    CC_WHITE       = 15,
    CC_MAGENTA     = 13,
    CC_DARK_GREEN  = 2,
    CC_DARK_RED    = 4,
    CC_DARK_YELLOW = 6,
    CC_DARK_WHITE  = 8,
    CC_BRIGHT_GREEN = 10,
    CC_BRIGHT_CYAN  = 11,
    CC_BRIGHT_RED   = 12,
    CC_BRIGHT_WHITE = 15,
    CC_VIOLET       = 13,
    CC_DARK_VIOLET  = 5,
    CC_GRAY         = 8,
    CC_LIGHT_GRAY   = 7,
    CC_BLUE         = 9,
    CC_DARK_BLUE    = 1
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

static void SetConsoleColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
}

static std::wstring ReadLine() {
    std::wstring line;
    std::getline(std::wcin, line);
    while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n')) {
        line.pop_back();
    }
    return line;
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

struct OptionData {
    std::wstring label;
    std::vector<std::wstring> commands; 
    int runMode = 0; 
};

// ============================================================
// AutoSave
// ============================================================
static std::wstring GetAppDataDir() {
    wchar_t appData[MAX_PATH];
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", appData, MAX_PATH) > 0) {
        std::wstring dir = std::wstring(appData) + L"\\BatchMenuGenerator\\";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir;
    }
    wchar_t tempDir[MAX_PATH];
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring dir = std::wstring(tempDir) + L"BatchMenuGenerator\\";
    CreateDirectoryW(dir.c_str(), NULL);
    return dir;
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

// ============================================================
// The actual BatchMenuGenerator
// ============================================================
static std::wstring GenerateBatch(const std::wstring& title,
    const std::vector<std::wstring>& description,
    const std::vector<OptionData>& options,
    const std::wstring& password,
    bool requireAdmin,
    wchar_t borderChar) {

    std::wstring border = MakeBorder(borderChar);
    if (border.empty()) border = MakeBorder(L'=');

    std::wstring bat;
    bat.reserve(8192);
    bat += L"Created with BatchMenuGenerator!\r\n";
    bat += L"@echo off\r\n";

    if (requireAdmin) {
        bat += L":: Admin check\r\n";
        bat += L"net session >nul 2>&1\r\n";
        bat += L"if %errorLevel% neq 0 (\r\n";
        bat += L"    echo " + border + L"\r\n";
        bat += L"    echo  Requires administrator privileges!\r\n";
        bat += L"    echo  Relaunching as Admin...\r\n";
        bat += L"    echo " + border + L"\r\n";
        bat += L"    timeout /t 5 >nul\r\n";
        bat += L"    powershell -Command \"Start-Process -FilePath '%~f0' -Verb RunAs\"\r\n";
        bat += L"    exit /b\r\n";
        bat += L")\r\n\r\n";
    }

    if (!password.empty()) {
        bat += L":auth_gate\r\ncls\r\n";
        bat += L"set /p \"pass_input=Enter password: \"\r\n";
        bat += L"if \"%pass_input%\"==\"" + EscapeBatch(password, false) + L"\" goto menu\r\n";
        bat += L"echo [!] Incorrect password.\r\npause\r\ngoto auth_gate\r\n\r\n";
    }

    bat += L"title " + title + L"\r\n:menu\r\ncls\r\n";
    bat += L"echo " + border + L"\r\n";
    
    const int BANNER_WIDTH = 57;
    int titleLen = (int)title.length();
    int padLeft = (BANNER_WIDTH - titleLen) / 2;
    if (padLeft < 0) padLeft = 0;
    
    std::wstring spaces(padLeft, L' ');
    bat += L"echo " + spaces + title + L"\r\n";
    bat += L"echo " + border + L"\r\necho.\r\n";

    if (!description.empty()) {
        for (const auto& line : description) {
            if (line.empty()) {
                bat += L"echo.\r\n";
            } else {
                bat += L"echo  " + EscapeBatch(line, false) + L"\r\n";
            }
        }
        bat += L"echo.\r\n";
        bat += L"echo " + border + L"\r\necho.\r\n";
    }

    for (size_t i = 0; i < options.size(); i++) {
        std::wstring l = options[i].label.empty() ? L"Option" : options[i].label;
        wchar_t b[256];
        swprintf(b, 256, L"echo  %zu) %s\r\n", i + 1, l.c_str());
        bat += b;
    }
    bat += L"echo  X) Exit\r\necho.\r\n";
    bat += L"echo " + border + L"\r\necho.\r\n";

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

static bool WriteScriptToFile(const std::wstring& path, const std::wstring& script) {
    HANDLE hf = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) return false;
    DWORD wr;
    BYTE bom[] = { 0xEF, 0xBB, 0xBF };
    WriteFile(hf, bom, 3, &wr, NULL);
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

static bool SaveToManualPath(const std::wstring& script, std::wstring& outSavedPath, std::wstring forcedFile = L"", std::wstring forcedDir = L"") {
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
                    wprintf(L"    1) Retype file details\n    2) Create folder automatically\n  Select option (1-2) [1]: "); C_I;
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
                            C_E; wprintf(L"  [Error] Failed to create directory.\n"); C_R; continue;
                        }
                    } else { continue; }
                }
            }
        }
        if (forcedFile.empty()) {
            DWORD fileAttr = GetFileAttributesW(fullPath.c_str());
            if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
                C_E; wprintf(L"  [Warning] File already exists!\n"); C_P;
                wprintf(L"    1) Replace\n    2) Rename to %ls1.bat\n    3) Retype\n  Select (1-3) [1]: ", baseName.c_str()); C_I;
                std::wstring opt = ReadLine();
                if (opt == L"2") fullPath = dir + baseName + L"1.bat";
                else if (opt == L"3") continue;
            }
        }
        if (WriteScriptToFile(fullPath, script)) {
            C_S; wprintf(L"  [SUCCESS] Saved to: %ls\n", fullPath.c_str()); C_R;
            outSavedPath = fullPath; return true;
        } else {
            DWORD err = GetLastError();
            C_E; wprintf(L"  [ERROR] Failed to save. (Code: %lu)\n", err); C_R;
            if (forcedFile.empty()) {
                C_P; wprintf(L"    1) Retry\n    2) Use Explorer dialog\n  Select (1-2) [2]: "); C_I;
                std::wstring fc = ReadLine(); if (fc.empty()) fc = L"2";
                if (fc == L"2") {
                    wchar_t path[MAX_PATH] = {0};
                    OPENFILENAMEW of; ZeroMemory(&of, sizeof(of));
                    of.lStructSize = sizeof(of); of.lpstrFilter = L"Batch Files (*.bat)\0*.bat\0All Files (*.*)\0*.*\0\0";
                    of.lpstrFile = path; of.nMaxFile = MAX_PATH; of.lpstrDefExt = L"bat";
                    of.lpstrTitle = L"Save Batch Menu Script"; of.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
                    if (GetSaveFileNameW(&of) && WriteScriptToFile(path, script)) {
                        C_S; wprintf(L"  [SUCCESS] Saved to: %ls\n", path); C_R;
                        outSavedPath = path; return true;
                    }
                    C_E; wprintf(L"  [Error] Explorer save failed.\n");
                }
            } else return false;
        }
    }
}

// ============================================================
// .txt config parser (loader)
// ============================================================
static bool ParseScriptConfigLines(const std::vector<std::wstring>& lines, std::wstring& outTitle, 
    std::vector<std::wstring>& outDesc, std::wstring& outPass, std::vector<OptionData>& outOptions,
    std::wstring& autoFilename, std::wstring& autoPath, bool& silentPreviewOnly,
    bool& outRequireAdmin) {

    int optionCount = 0;
    int currentOption = -1;
    bool collecting = false;
    outRequireAdmin = false;

    for (const auto& rawLine : lines) {
        std::wstring wLine = Trim(rawLine);
        if (wLine.empty() || wLine.substr(0, 2) == L"//") continue;

        if (collecting && wLine == L"}") { collecting = false; continue; }
        if (collecting && currentOption >= 0 && currentOption < (int)outOptions.size()) {
            if (wLine.size() >= 8 && wLine.substr(0, 8) == L"runmode ") {
                int mode = _wtoi(Trim(wLine.substr(8)).c_str());
                outOptions[currentOption].runMode = (mode == 1) ? 1 : 0;
            } else {
                outOptions[currentOption].commands.push_back(wLine);
            }
            continue;
        }

        if (wLine.size() >= 6 && wLine.substr(0, 6) == L"title ") outTitle = Trim(wLine.substr(6));
        else if (wLine.size() >= 12 && wLine.substr(0, 12) == L"description ") { std::wstring d = Trim(wLine.substr(12)); if (d != L"n" && d != L"no") outDesc.push_back(d); }
        else if (wLine.size() >= 6 && wLine.substr(0, 6) == L"admin ") { std::wstring a = Trim(wLine.substr(6)); outRequireAdmin = (a == L"y" || a == L"Y" || a == L"yes"); }
        else if (wLine.size() >= 5 && wLine.substr(0, 5) == L"pass ") { std::wstring p = Trim(wLine.substr(5)); if (p == L"n" || p == L"no" || p.empty()) outPass = L""; else outPass = p; }
        else if (wLine.size() >= 12 && wLine.substr(0, 12) == L"option num: ") { optionCount = _wtoi(Trim(wLine.substr(12)).c_str()); if (optionCount < 1) optionCount = 1; if (optionCount > 20) optionCount = 20; outOptions.resize(optionCount); }
        else if (wLine.size() >= 7 && wLine.substr(0, 7) == L"option ") {
            size_t lbl = wLine.find(L" label "); size_t brc = wLine.find(L" {");
            if (lbl != std::wstring::npos) { int num = _wtoi(wLine.substr(7, lbl - 7).c_str()); if (num >= 1 && num <= optionCount) outOptions[num - 1].label = Trim(wLine.substr(lbl + 7)); }
            else if (brc != std::wstring::npos) { int num = _wtoi(wLine.substr(7, brc - 7).c_str()); if (num >= 1 && num <= optionCount) { currentOption = num - 1; collecting = true; } }
        }
        else if (wLine.size() >= 8 && wLine.substr(0, 8) == L"runmode ") { int mode = _wtoi(Trim(wLine.substr(8)).c_str()); if (currentOption >= 0 && currentOption < (int)outOptions.size()) outOptions[currentOption].runMode = (mode == 1) ? 1 : 0; }
        else if (wLine.size() >= 5 && wLine.substr(0, 5) == L"save ") {
            std::wstring args = Trim(wLine.substr(5));
            if (args == L"n") silentPreviewOnly = true;
            else {
                size_t pos1 = args.find(L"--");
                if (pos1 != std::wstring::npos) {
                    size_t pos2 = args.find(L"--", pos1 + 2);
                    if (pos2 != std::wstring::npos) { autoFilename = Trim(args.substr(pos1 + 2, pos2 - (pos1 + 2))); autoPath = Trim(args.substr(pos2 + 2)); }
                    else autoFilename = Trim(args.substr(pos1 + 2));
                }
            }
        }
    }
    return true;
}

static bool ParseScriptConfig(const std::wstring& filePath, std::wstring& outTitle, 
    std::vector<std::wstring>& outDesc, std::wstring& outPass, std::vector<OptionData>& outOptions,
    std::wstring& autoFilename, std::wstring& autoPath, bool& silentPreviewOnly,
    bool& outRequireAdmin) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;
    std::string line; std::vector<std::wstring> lines;
    while (std::getline(file, line)) lines.push_back(ConvertToWide(line));
    file.close();
    return ParseScriptConfigLines(lines, outTitle, outDesc, outPass, outOptions, autoFilename, autoPath, silentPreviewOnly, outRequireAdmin);
}

// ============================================================
// Command mode (accesible via debug mode)
// ============================================================
static bool ConsoleTextEditor(std::vector<std::wstring>& lines) {
    if (lines.empty()) lines.push_back(L"");
    int curLine = (int)lines.size() - 1;
    auto Redraw = [&]() {
        COORD cursorPosition; cursorPosition.X = 0; cursorPosition.Y = 0;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);
        C_H; wprintf(L" --- Text Mode (ESC for menu, Up/Down to navigate) ---\n\n");
        for (size_t i = 0; i < lines.size(); i++) {
            wprintf(L"                                                       \r");
            if ((int)i == curLine) { C_A; wprintf(L" > %ls_\n", lines[i].c_str()); }
            else { C_T; wprintf(L"   %ls\n", lines[i].c_str()); }
        }
        wprintf(L"                                                             \n"); C_R;
    };
    Redraw();
    while (true) {
        int c = _getwch();
        if (c == 27) {
            C_N; wprintf(L"\n\n [ESC Menu] 1: Continue | 2: Compile (finish) | 3: Exit Program: "); C_R;
            while(true) { int opt = _getwch(); if (opt == L'1') { Redraw(); break; } if (opt == L'2') return true; if (opt == L'3') exit(0); }
        } else if (c == 13) { if (curLine == (int)lines.size() - 1) lines.push_back(L""); else lines.insert(lines.begin() + curLine + 1, L""); curLine++; Redraw(); }
        else if (c == 8) { if (!lines[curLine].empty()) { lines[curLine].pop_back(); Redraw(); } else if (curLine > 0) { lines.erase(lines.begin() + curLine); curLine--; Redraw(); } }
        else if (c == 224 || c == 0) { int c2 = _getwch(); if (c2 == 72 && curLine > 0) { curLine--; Redraw(); } else if (c2 == 80 && curLine < (int)lines.size() - 1) { curLine++; Redraw(); } }
        else if (c >= 32) { lines[curLine].push_back((wchar_t)c); Redraw(); }
    }
    return false;
}

// ============================================================
// Debug menu toggles
// ============================================================
static bool g_enableTheme = false;
static bool g_enableAutoSave = true; 
static bool g_enableBConfig = false;
static bool g_enableEscPause = false;
static bool g_enableCommandMode = false;
static bool g_debugMode = false;

// ============================================================
// AutoSave resume step system
// ============================================================
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
    }
    
    std::wstring sessionTimestamp;
    { SYSTEMTIME st; GetLocalTime(&st); wchar_t buf[64]; swprintf(buf, 64, L"%02d:%02d %02d/%02d/%02d", st.wHour, st.wMinute, st.wDay, st.wMonth, st.wYear % 100); sessionTimestamp = buf; }
    
    auto SaveStep = [&](const std::wstring& stepCode) {
        if (!g_enableAutoSave) return;
        std::vector<std::wstring> lines;
        lines.push_back(L"// Autosave from " + sessionTimestamp);
        lines.push_back(L"step " + stepCode);
        lines.push_back(L"title " + title);
        for (const auto& d : description) lines.push_back(L"description " + d);
        if (!password.empty()) lines.push_back(L"pass " + password);
        lines.push_back(requireAdmin ? L"admin y" : L"admin n");
        lines.push_back(L"option num: " + std::to_wstring(options.size()));
        for (size_t oi = 0; oi < options.size(); oi++) {
            std::wstring optNum = std::to_wstring(oi + 1);
            if (!options[oi].label.empty()) {
                lines.push_back(L"option " + optNum + L" label " + options[oi].label);
            }
            lines.push_back(L"option " + optNum + L" {");
            lines.push_back(L"runmode " + std::to_wstring(options[oi].runMode));
            for (const auto& c : options[oi].commands) {
                lines.push_back(c);
            }
            lines.push_back(L"}");
        }
        WriteAutoSave(lines);
    };

    if (resumeStage < 1) {
        while (true) {
            C_P; wprintf(L"  Enter window title (Max 57 chars): "); C_I;
            title = ReadLine(); C_R;
            if (title.empty()) { title = L"Batch Menu"; break; }
            if (title.length() <= 57) break;
            else { C_E; wprintf(L"  Title invalid: cannot be longer than 57 characters (including spaces)!\n"); C_R; }
        }
        SaveStep(L"TITLE_DONE");
    }

    if (resumeStage < 2) {
        C_P; wprintf(L"  Do you want to add a description? (Y/n): "); C_I;
        std::wstring descAns = ReadLine(); C_R;
        if (descAns.empty() || descAns == L"Y" || descAns == L"y" || descAns == L"yes") {
            C_P; wprintf(L"  Enter description lines below.\n  Press Enter on an empty line when done.\n"); C_S; wprintf(L"  Description:\n"); C_R;
            while (true) { C_I; wprintf(L"    > "); std::wstring descLine = ReadLine(); C_R; if (descLine.empty()) break; description.push_back(descLine); SaveStep(L"DESC_LINE"); }
        }
        SaveStep(L"DESC_DONE");
    }

    if (resumeStage < 3) {
        C_P; wprintf(L"\n  [Note] Password protecting a batch file is not fully secure since anyone can right-click and edit it.\n");
        wprintf(L"  You can use a tool like Bat-To-Exe-Converter (found on GitHub) to turn your script into a secure .exe.\n");
        wprintf(L"  Enter password (leave empty for none): "); C_I;
        password = ReadLine(); C_R;
        SaveStep(L"PASS_DONE");
    }

    if (resumeStage < 4) {
        C_P; wprintf(L"\n  Will this script require Administrative privileges? (y/N): "); C_I;
        std::wstring adminAns = ReadLine(); C_R;
        requireAdmin = (!adminAns.empty() && (adminAns == L"y" || adminAns == L"Y" || adminAns == L"yes"));
        if (requireAdmin) { C_S; wprintf(L"  [Admin elevation code will be added to the script]\n"); C_R; }
        SaveStep(L"ADMIN_DONE");
    }

    if (resumeStage < 5) {
        C_P; wprintf(L"  Number of options (1-20) [3]: "); C_I;
        std::wstring numStr = ReadLine(); C_R;
        int optionCount = 3;
        if (!numStr.empty()) { wchar_t* end = nullptr; int val = (int)wcstol(numStr.c_str(), &end, 10); if (end && *end == L'\0' && val >= 1 && val <= 20) optionCount = val; }
        options.resize(optionCount);
        SaveStep(L"OPTCOUNT_DONE");
    }

    for (size_t i = resumeOptIndex; i < options.size(); i++) {
        C_N; wprintf(L"\n  --- Option %zu ---\n", i + 1); C_R;
        
        if (!options[i].label.empty()) {
            C_P; wprintf(L"  Label [Option %zu] [%ls]: ", i + 1, options[i].label.c_str()); C_I;
            std::wstring label = ReadLine(); C_R;
            if (!label.empty()) options[i].label = label;
        } else {
            C_P; wprintf(L"  Label [Option %zu]: ", i + 1); C_I;
            std::wstring label = ReadLine(); C_R;
            options[i].label = label.empty() ? L"Option" : label;
        }
        SaveStep(L"OPT_LABEL_" + std::to_wstring(i));

        C_P; wprintf(L"  Run mode:\n    0 = Direct call (%%command%%)\n    1 = cmd /c wrapper\n  Select (0/1) [%d]: ", options[i].runMode); C_I;
        std::wstring modeStr = ReadLine(); C_R;
        if (!modeStr.empty()) {
            options[i].runMode = (modeStr == L"1") ? 1 : 0;
        }
        SaveStep(L"OPT_RUNMODE_" + std::to_wstring(i));

        if (!options[i].commands.empty()) {
            C_S; wprintf(L"  Existing commands for Option %zu:\n", i + 1);
            for (const auto& c : options[i].commands) {
                wprintf(L"    > %ls\n", c.c_str());
            }
            C_P; wprintf(L"  Add more command(s), one per line (Press Enter on empty line when done):\n"); C_R;
        } else {
            C_P; wprintf(L"  Enter command(s), one per line.\n  Press Enter on an empty line when done.\n"); C_S; wprintf(L"  Commands:\n"); C_R;
        }
        
        while (true) {
            C_I; wprintf(L"    > "); std::wstring cmd = ReadLine(); C_R;
            if (cmd.empty()) break;
            options[i].commands.push_back(cmd);
            SaveStep(L"OPT_CMD_" + std::to_wstring(i));
        }
        SaveStep(L"OPT_DONE_" + std::to_wstring(i));
    }
    SaveStep(L"ALL_OPTIONS_DONE");
    return true;
}

// ============================================================
// Tester Menu (does not work yet) and Debug mode (works)
// ============================================================
static void RunTesterMenu() {
    system("cls");
    C_B; wprintf(L"  ============================================\n"); C_N;
    wprintf(L"           TESTER MENU\n"); C_B;
    wprintf(L"  ============================================\n\n"); C_T;
    wprintf(L"  Tester options (simulates program flows):\n\n");
    C_P; wprintf(L"    1) Generate batch with all features\n    2) Generate batch with admin check only\n");
    wprintf(L"    3) Generate batch with password only\n    4) Generate batch with custom border\n");
    wprintf(L"    5) Run SaveToManualPath test\n    6) Run ParseScriptConfig test\n    0) Return\n\n"); C_P;
    wprintf(L"  Select option: "); C_I; int tc = _getwch(); C_T;
    if (tc >= L'1' && tc <= L'6') { C_S; wprintf(L"\n  [Test initiated - feature under construction]\n"); C_R; Sleep(1000); }
    C_R; system("cls");
}

static void RunDebugMenu() {
    system("cls");
    C_B; wprintf(L"  ============================================\n"); C_N;
    wprintf(L"           DEBUG MODE - TESTING MENU\n"); C_B;
    wprintf(L"  ============================================\n\n"); C_T;
    wprintf(L"  Feature Toggles:\n\n");
    C_P; wprintf(L"    1) Theme Engine        : "); C_I; wprintf(L"%s\n", g_enableTheme ? L"ENABLED" : L"disabled"); C_T;
    C_P; wprintf(L"    2) AutoSave System      : "); C_I; wprintf(L"%s\n", g_enableAutoSave ? L"ENABLED" : L"disabled"); C_T;
    C_P; wprintf(L"    3) BConfig System       : "); C_I; wprintf(L"%s\n", g_enableBConfig ? L"ENABLED" : L"disabled"); C_T;
    C_P; wprintf(L"    4) ESC Pause/Back-Step  : "); C_I; wprintf(L"%s\n", g_enableEscPause ? L"ENABLED" : L"disabled"); C_T;
    C_P; wprintf(L"    5) Command Mode         : "); C_I; wprintf(L"%s\n", g_enableCommandMode ? L"ENABLED" : L"disabled"); C_T;
    C_P; wprintf(L"    6) Toggle All On\n    7) Toggle All Off\n    8) Launch Tester\n\n"); C_T;
    C_P; wprintf(L"  Select option (0 to exit): "); C_I;
    int choice = _getwch(); C_T;
    if (choice == L'1') { g_enableTheme = !g_enableTheme; RunDebugMenu(); return; }
    if (choice == L'2') { g_enableAutoSave = !g_enableAutoSave; RunDebugMenu(); return; }
    if (choice == L'3') { g_enableBConfig = !g_enableBConfig; RunDebugMenu(); return; }
    if (choice == L'4') { g_enableEscPause = !g_enableEscPause; RunDebugMenu(); return; }
    if (choice == L'5') { g_enableCommandMode = !g_enableCommandMode; RunDebugMenu(); return; }
    if (choice == L'6') { g_enableTheme = g_enableAutoSave = g_enableBConfig = g_enableEscPause = g_enableCommandMode = true; RunDebugMenu(); return; }
    if (choice == L'7') { g_enableTheme = g_enableAutoSave = g_enableBConfig = g_enableEscPause = g_enableCommandMode = false; RunDebugMenu(); return; }
    if (choice == L'8') { RunTesterMenu(); return; }
    C_R; system("cls");
}

// ============================================================
// Main entry
// ============================================================
int main() {
    SetConsoleTitle(L"BatchMenuGeneratorCLI");
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    std::wstring autoSavePath = GetAutoSavePath();
    std::wstring autoSaveTimestamp = ReadAutoSaveTimestamp(autoSavePath);
    std::vector<std::wstring> autosaveData = ReadAutoSave(autoSavePath);
    bool hasAutosave = !autosaveData.empty();

    while (true) {
        if (!g_debugMode) system("cls");

        C_B; wprintf(L"\n  ============================================\n"); C_B;
        wprintf(L"       Batch Menu Generator - CLI Edition\n");
        if (g_debugMode) { C_E; wprintf(L"              ** DEBUG MODE **\n"); }
        C_B; wprintf(L"  ============================================\n\n"); C_R;

        if (hasAutosave) {
            C_N; wprintf(L"  [Autosave found from %ls]\n", autoSaveTimestamp.c_str());
            wprintf(L"  Would you like to continue generating the batch script? (Y/n): "); C_I;
            std::wstring asAns = ReadLine(); C_R;
            if (asAns.empty() || asAns == L"Y" || asAns == L"y" || asAns == L"yes") {
                std::wstring savedTitle;
                std::vector<std::wstring> savedDesc;
                std::wstring savedPass;
                std::vector<OptionData> savedOptions;
                std::wstring savedAutoFilename, savedAutoPath;
                bool savedSilentPreview = false, savedRequireAdmin = false;
                std::wstring savedStep = L"";
                
                for (const auto& line : autosaveData) {
                    std::wstring wl = Trim(line);
                    if (wl.substr(0, 5) == L"step ") savedStep = Trim(wl.substr(5));
                }
                
                if (ParseScriptConfigLines(autosaveData, savedTitle, savedDesc, savedPass, savedOptions, savedAutoFilename, savedAutoPath, savedSilentPreview, savedRequireAdmin)) {
                    C_S; wprintf(L"  [Autosave restored - resuming at step: %ls]\n", savedStep.c_str()); C_R;
                    Sleep(1000);
                    
                    // Run the Mode 1 flow from the saved step
                    wchar_t bc = L'=';
                    bool completed = RunMode1Flow(savedTitle, savedDesc, savedPass, savedOptions, savedRequireAdmin, bc, savedStep);
                    DeleteAutoSave(autoSavePath);
                    hasAutosave = false;
                    
                    if (completed) {
                        // Go to generation
                        std::wstring script = GenerateBatch(savedTitle, savedDesc, savedOptions, savedPass, savedRequireAdmin, bc);
                        C_B; wprintf(L"\n  ========== PREVIEW ==========\n"); C_R;
                        wprintf(L"%ls\n", script.c_str());
                        
                        std::wstring finalSavedPath = L"", tempFilePath = L"";
                        C_P; wprintf(L"  Save to file? (Y/n): "); C_I;
                        std::wstring saveAns = ReadLine(); C_R;
                        if (saveAns.empty() || saveAns == L"Y" || saveAns == L"y" || saveAns == L"yes")
                            SaveToManualPath(script, finalSavedPath);
                        
                        // End menu
                        while (true) {
                            C_P; wprintf(L"\n  ============================================\n");
                            wprintf(L"  What would you like to do next?\n    1) Main menu\n    2) Launch\n    3) Exit\n  Select [3]: "); C_I;
                            std::wstring endChoice = ReadLine(); C_R;
                            if (endChoice == L"3" || endChoice.empty()) { DeleteTempFile(tempFilePath); break; }
                            else if (endChoice == L"2") {
                                if (finalSavedPath.empty()) {
                                    tempFilePath = GenerateTempFilePath();
                                    if (!WriteScriptToFile(tempFilePath, script)) { C_E; wprintf(L"  [Error] Could not create temp file!\n"); C_R; continue; }
                                    finalSavedPath = tempFilePath;
                                }
                                ShellExecuteW(NULL, L"open", finalSavedPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                Sleep(3000); DeleteTempFile(tempFilePath); tempFilePath = L"";
                            } else break;
                        }
                    }
                    system("cls"); continue;
                } else { C_E; wprintf(L"  [Autosave data corrupted, starting fresh]\n"); C_R; }
            }
            DeleteAutoSave(autoSavePath);
            hasAutosave = false;
            system("cls"); continue;
        }

        C_P; wprintf(L"  Select Mode:\n    1 = Make new Batch Menu\n    2 = Load TXT Config\n");
        if (g_enableCommandMode) wprintf(L"    3 = Command Mode\n");
        if (g_enableTheme) wprintf(L"    T = Change Theme\n");
        wprintf(L"  Choice (1-3) [1]: "); C_I;
        std::wstring workflowChoice = ReadLine(); C_R;
        if (workflowChoice.empty()) workflowChoice = L"1";

        if (workflowChoice == L"3" && !g_enableCommandMode) {
            C_E; wprintf(L"\n  Command Mode is disabled. Enable it in Debug Menu (2211).\n"); C_R;
            system("pause"); system("cls"); continue;
        }

        if (g_enableTheme && (workflowChoice == L"T" || workflowChoice == L"t")) {
            system("cls");
            C_B; wprintf(L"  ============================================\n"); C_N;
            wprintf(L"           CHANGE THEME\n"); C_B;
            wprintf(L"  ============================================\n\n"); C_T;
            const wchar_t* themes[] = { L"default", L"matrix", L"cyberpunk", L"cli", L"violent" };
            for (int i = 0; i < 5; i++) { ApplyTheme(themes[i]); C_P; wprintf(L"    %d) ", i + 1); C_H; wprintf(L"[%ls]\n", themes[i]); }
            ApplyTheme(L"default");
            C_P; wprintf(L"\n  Select theme (1-5, 0 to cancel): "); C_I;
            int thc = _getwch();
            if (thc >= L'1' && thc <= L'5') { int idx = thc - L'1'; const wchar_t* chosen[] = { L"default", L"matrix", L"cyberpunk", L"cli", L"violent" }; ApplyTheme(chosen[idx]); C_S; wprintf(L"\n  Theme set to: %ls\n", chosen[idx]); C_R; Sleep(1000); }
            C_R; system("cls"); continue;
        }

        if (workflowChoice == L"2211") { g_debugMode = !g_debugMode; if (g_debugMode) RunDebugMenu(); continue; }

        std::wstring title;
        std::vector<std::wstring> description;
        std::wstring password = L"";
        std::vector<OptionData> options;
        std::wstring autoFilename = L"", autoPath = L"";
        bool silentPreviewOnly = false, requireAdmin = false;
        std::wstring finalSavedPath = L"", tempFilePath = L"";
        wchar_t borderChar = L'=';

        if (workflowChoice == L"2") {
            bool loaded = false; std::wstring loadChoice = L"";
            while (!loaded) {
                if (loadChoice.empty()) {
                    C_P; wprintf(L"\n  Choose Load Method:\n    1 = Browse (Windows File Explorer dialog)\n    2 = Type path manually\n  Select (1/2) [1]: "); C_I;
                    loadChoice = ReadLine(); C_R; if (loadChoice.empty()) loadChoice = L"1";
                }
                if (loadChoice != L"1" && loadChoice != L"2") { C_E; wprintf(L"  Invalid option choice.\n"); C_R; loadChoice = L""; continue; }
                std::wstring configPath = L"";
                if (loadChoice == L"1") {
                    wchar_t path[MAX_PATH] = {0}; OPENFILENAMEW of; ZeroMemory(&of, sizeof(of));
                    of.lStructSize = sizeof(of); of.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
                    of.lpstrFile = path; of.nMaxFile = MAX_PATH; of.lpstrDefExt = L"txt";
                    of.lpstrTitle = L"Select Configuration File"; of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                    if (GetOpenFileNameW(&of)) configPath = path;
                    else { C_P; wprintf(L"\n  Dialog canceled. Defaulting to manual path entry...\n"); C_R; loadChoice = L"2"; continue; }
                } else { C_P; wprintf(L"\n  Enter config file path: "); C_I; configPath = ReadLine(); C_R; }
                if (configPath.length() >= 2 && configPath.front() == L'"' && configPath.back() == L'"')
                    configPath = configPath.substr(1, configPath.length() - 2);
                if (configPath.empty() || !ParseScriptConfig(configPath, title, description, password, options, autoFilename, autoPath, silentPreviewOnly, requireAdmin)) {
                    C_E; wprintf(L"  [Error] Failed to load config file.\n"); C_P;
                    wprintf(L"    1) Try again\n    2) Browse\n    3) Cancel\n  Select (1-3) [1]: "); C_I; C_R;
                    std::wstring retryAns = ReadLine();
                    if (retryAns == L"2") loadChoice = L"1"; else if (retryAns == L"3") break; else loadChoice = L"2";
                } else loaded = true;
            }
            if (!loaded) { system("cls"); continue; }
        } else if (workflowChoice == L"3") {
            std::vector<std::wstring> editorLines;
            bool compile = ConsoleTextEditor(editorLines);
            if (!compile) { system("cls"); continue; }
            if (!ParseScriptConfigLines(editorLines, title, description, password, options, autoFilename, autoPath, silentPreviewOnly, requireAdmin)) {
                C_E; wprintf(L"  [Error] Failed to process editor text!\n"); C_R;
                system("pause"); system("cls"); continue;
            }
        } else {
            // Mode 1: run flow and clear autosave on completion
            if (!RunMode1Flow(title, description, password, options, requireAdmin, borderChar)) {
                system("cls"); continue;
            }
            DeleteAutoSave(autoSavePath);
        }

        // Border style prompt
        C_P; wprintf(L"\n  Select border style:\n    1) Classic  :  =========================================================\n");
        wprintf(L"    2) Modern   :  ---------------------------------------------------------\n");
        wprintf(L"    3) Retro    :  |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
        wprintf(L"    4) Custom   :  (Enter any character you want)\n  Select (1-4) [1]: "); C_I;
        std::wstring borderChoice = ReadLine(); C_R;
        if (borderChoice == L"2") borderChar = L'-';
        else if (borderChoice == L"3") borderChar = L'|';
        else if (borderChoice == L"4") { C_P; wprintf(L"  Enter border character: "); C_I; std::wstring custBorder = ReadLine(); C_R; if (!custBorder.empty()) borderChar = custBorder[0]; }

        C_H; wprintf(L"\n  Generating batch script...\n"); C_R;
        std::wstring script = GenerateBatch(title, description, options, password, requireAdmin, borderChar);
        C_B; wprintf(L"\n  ========== PREVIEW ==========\n"); C_R;
        wprintf(L"%ls\n", script.c_str());

        if ((workflowChoice == L"2" || workflowChoice == L"3") && silentPreviewOnly) {
            C_P; wprintf(L"\n  Not saved! Exiting preview mode.\n"); C_R;
        } else if ((workflowChoice == L"2" || workflowChoice == L"3") && !autoFilename.empty()) {
            SaveToManualPath(script, finalSavedPath, autoFilename, autoPath);
        } else {
            C_P; wprintf(L"  Save to file? (Y/n): "); C_I;
            std::wstring saveAns = ReadLine(); C_R;
            if (saveAns.empty() || saveAns == L"Y" || saveAns == L"y" || saveAns == L"yes") {
                std::wstring methodChoice;
                while (true) {
                    C_P; wprintf(L"  Choose Save Method:\n    1 = Type path manually\n    2 = Browse (Windows File Explorer dialog)\n  Select (1/2) [2]: "); C_I;
                    methodChoice = ReadLine(); C_R;
                    if (methodChoice.empty()) methodChoice = L"2";
                    if (methodChoice == L"1" || methodChoice == L"2") break;
                    C_E; wprintf(L"  Invalid option choice.\n"); C_R;
                }
                if (methodChoice == L"1") SaveToManualPath(script, finalSavedPath);
                else {
                    wchar_t path[MAX_PATH] = {0}; OPENFILENAMEW of; ZeroMemory(&of, sizeof(of));
                    of.lStructSize = sizeof(of); of.lpstrFilter = L"Batch Files (*.bat)\0*.bat\0All Files (*.*)\0*.*\0\0";
                    of.lpstrFile = path; of.nMaxFile = MAX_PATH; of.lpstrDefExt = L"bat";
                    of.lpstrTitle = L"Save Batch Menu Script"; of.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
                    if (GetSaveFileNameW(&of)) {
                        if (!WriteScriptToFile(path, script)) { C_E; wprintf(L"\n  [ERROR] Explorer save failed.\n"); C_R; SaveToManualPath(script, finalSavedPath); }
                        else { C_S; wprintf(L"\n  [SUCCESS] Saved to: %ls\n", path); C_R; finalSavedPath = path; }
                    } else { C_P; wprintf(L"\n  Dialog canceled. Defaulting to manual path entry...\n"); C_R; SaveToManualPath(script, finalSavedPath); }
                }
            } else { C_P; wprintf(L"\n  Not saved!\n"); C_R; }
        }

        C_P; wprintf(L"\n  ============================================\n");
        wprintf(L"  What would you like to do next?\n    1) Go back to main menu\n    2) Launch script\n    3) Exit\n  Select [3]: "); C_I;
        std::wstring endChoice = ReadLine(); C_R;

        if (endChoice == L"3" || endChoice.empty()) { DeleteTempFile(tempFilePath); break; }
        else if (endChoice == L"2") {
            if (finalSavedPath.empty()) {
                tempFilePath = GenerateTempFilePath();
                if (!WriteScriptToFile(tempFilePath, script)) { C_E; wprintf(L"  [Error] Could not create temp file!\n"); C_R; system("pause"); continue; }
                finalSavedPath = tempFilePath;
                C_S; wprintf(L"\n  [Temp file saved to: %ls]\n", tempFilePath.c_str()); C_R;
            }
            ShellExecuteW(NULL, L"open", finalSavedPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            C_P; wprintf(L"\n  Cleaning up temp file...\n"); C_R; Sleep(3000); DeleteTempFile(tempFilePath); tempFilePath = L"";
        }
        system("cls");
    }
    return 0;
}
