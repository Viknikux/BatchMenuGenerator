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
#include <fstream>
#include <sstream>
#include <io.h>
#include <fcntl.h>

#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")

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

static std::wstring Trim(const std::wstring& str) {
    size_t first = str.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    size_t last = str.find_last_not_of(L" \t\r\n");
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

static std::wstring GenerateBatch(const std::wstring& title,
    const std::vector<std::wstring>& description,
    const std::vector<OptionData>& options,
    const std::wstring& password) {

    std::wstring bat;
    bat.reserve(8192);
    bat += L"Created with BatchMenuGenerator!\r\n";
    bat += L"@echo off\r\n";

    if (!password.empty()) {
        bat += L":auth_gate\r\ncls\r\n";
        bat += L"set /p \"pass_input=Enter password: \"\r\n";
        bat += L"if \"%pass_input%\"==\"" + EscapeBatch(password, false) + L"\" goto menu\r\n";
        bat += L"echo [!] Incorrect password.\r\npause\r\ngoto auth_gate\r\n\r\n";
    }

    bat += L"title " + title + L"\r\n:menu\r\ncls\r\n";
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

static bool SaveToManualPath(const std::wstring& script, std::wstring forcedFile = L"", std::wstring forcedDir = L"") {
    while (true) {
        std::wstring filename = forcedFile;
        if (filename.empty()) {
            SetConsoleColor(CC_YELLOW);
            wprintf(L"\n  Enter filename: ");
            SetConsoleColor(CC_RESET);
            filename = ReadLine();
            if (filename.empty()) {
                SetConsoleColor(CC_RED);
                wprintf(L"  [Error] Filename cannot be empty.\n");
                SetConsoleColor(CC_RESET);
                continue;
            }
        }

        if (filename.length() < 4 || _wcsicmp(filename.c_str() + filename.length() - 4, L".bat") != 0) {
            filename += L".bat";
        }

        std::wstring dir = forcedDir;
        if (dir.empty()) {
            SetConsoleColor(CC_YELLOW);
            wprintf(L"  Enter path [Press Enter for Current (program) Directory]: ");
            SetConsoleColor(CC_RESET);
            dir = ReadLine();
        }

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
                            wprintf(L"  [Error] Failed to create directory.\n");
                            SetConsoleColor(CC_RESET);
                            continue;
                        }
                    } else {
                        continue;
                    }
                }
            }
        }

        if (forcedFile.empty()) {
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
        }

        if (WriteScriptToFile(fullPath, script)) {
            SetConsoleColor(CC_GREEN);
            wprintf(L"  [SUCCESS] Saved to: %s\n", fullPath.c_str());
            SetConsoleColor(CC_RESET);
            return true;
        } else {
            SetConsoleColor(CC_RED);
            wprintf(L"  [ERROR] Write operations restricted at this path.\n");
            SetConsoleColor(CC_RESET);
            if (!forcedFile.empty()) return false;
        }
    }
}

static bool ParseScriptConfig(const std::wstring& filePath, std::wstring& outTitle, 
    std::vector<std::wstring>& outDesc, std::wstring& outPass, std::vector<OptionData>& outOptions,
    std::wstring& autoFilename, std::wstring& autoPath, bool& silentPreviewOnly) {

    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string line;
    int optionCount = 0;
    int currentOption = -1;
    bool collecting = false;

    while (std::getline(file, line)) {
        std::wstring wLine = Trim(ConvertToWide(line));
        if (wLine.empty() || wLine.substr(0, 2) == L"//") continue;

        if (collecting && wLine == L"}") {
            collecting = false;
            continue;
        }
        if (collecting && currentOption >= 0 && currentOption < (int)outOptions.size()) {
            outOptions[currentOption].commands.push_back(wLine);
            continue;
        }

        if (wLine.size() >= 6 && wLine.substr(0, 6) == L"title ") {
            outTitle = Trim(wLine.substr(6));
        } 
        else if (wLine.size() >= 12 && wLine.substr(0, 12) == L"description ") {
            std::wstring d = Trim(wLine.substr(12));
            if (d != L"n" && d != L"no") outDesc.push_back(d);
        } 
        else if (wLine.size() >= 5 && wLine.substr(0, 5) == L"pass ") {
            std::wstring p = Trim(wLine.substr(5));
            if (p == L"n" || p == L"no" || p.empty()) outPass = L"";
            else outPass = p;
        } 
        else if (wLine.size() >= 12 && wLine.substr(0, 12) == L"option num: ") {
            optionCount = _wtoi(Trim(wLine.substr(12)).c_str());
            if (optionCount < 1) optionCount = 1;
            if (optionCount > 20) optionCount = 20;
            outOptions.resize(optionCount);
        } 
        else if (wLine.size() >= 7 && wLine.substr(0, 7) == L"option ") {
            size_t lbl = wLine.find(L" label ");
            size_t brc = wLine.find(L" {");
            if (lbl != std::wstring::npos) {
                int num = _wtoi(wLine.substr(7, lbl - 7).c_str());
                if (num >= 1 && num <= optionCount) {
                    outOptions[num - 1].label = Trim(wLine.substr(lbl + 7));
                }
            } 
            else if (brc != std::wstring::npos) {
                int num = _wtoi(wLine.substr(7, brc - 7).c_str());
                if (num >= 1 && num <= optionCount) {
                    currentOption = num - 1;
                    collecting = true;
                }
            }
        } 
        else if (wLine.size() >= 8 && wLine.substr(0, 8) == L"runmode ") {
            int mode = _wtoi(Trim(wLine.substr(8)).c_str());
            if (currentOption >= 0 && currentOption < (int)outOptions.size()) {
                outOptions[currentOption].runMode = (mode == 1) ? 1 : 0;
            }
        } 
        else if (wLine.size() >= 5 && wLine.substr(0, 5) == L"save ") {
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
        }
    }
    file.close();
    return true;
}

int main() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    SetConsoleColor(CC_CYAN);
    wprintf(L"\n");
    wprintf(L"  ============================================\n");
    wprintf(L"       Batch Menu Generator - CLI Edition\n");
    wprintf(L"  ============================================\n\n");
    SetConsoleColor(CC_RESET);

    SetConsoleColor(CC_YELLOW);
    wprintf(L"  Select Mode:\n");
    wprintf(L"    1 = Make new Batch Menu\n");
    wprintf(L"    2 = Load TXT Config\n");
    wprintf(L"  Choice (1-2) [1]: ");
    SetConsoleColor(CC_RESET);
    std::wstring workflowChoice = ReadLine();
    if (workflowChoice.empty()) workflowChoice = L"1";

    std::wstring title;
    std::vector<std::wstring> description;
    std::wstring password = L"";
    std::vector<OptionData> options;

    std::wstring autoFilename = L"";
    std::wstring autoPath = L"";
    bool silentPreviewOnly = false;
    bool txtModeActive = (workflowChoice == L"2");

    if (txtModeActive) {
        SetConsoleColor(CC_YELLOW);
        wprintf(L"\n  Enter config file path: ");
        SetConsoleColor(CC_RESET);
        std::wstring configPath = ReadLine();
        
        if (configPath.length() >= 2 && configPath.front() == L'"' && configPath.back() == L'"') {
            configPath = configPath.substr(1, configPath.length() - 2);
        }

        if (!ParseScriptConfig(configPath, title, description, password, options, autoFilename, autoPath, silentPreviewOnly)) {
            SetConsoleColor(CC_RED);
            wprintf(L"  [Error] Failed to load config file.\n");
            SetConsoleColor(CC_RESET);
            ReadLine();
            return 1;
        }
    } 
    else {
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
        wprintf(L"\n  [Note] Password protecting a batch file is not fully secure since anyone can right-click and edit it.\n");
        wprintf(L"  You can use a tool like Bat-To-Exe-Converter (found on GitHub) to turn your script into a secure .exe.\n");
        wprintf(L"  Enter password (leave empty for none): ");
        SetConsoleColor(CC_RESET);
        password = ReadLine();

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

        options.resize(optionCount);

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
    }

    SetConsoleColor(CC_CYAN);
    wprintf(L"\n  Generating batch script...\n");
    SetConsoleColor(CC_RESET);

    std::wstring script = GenerateBatch(title, description, options, password);

    SetConsoleColor(CC_WHITE);
    wprintf(L"\n  ========== PREVIEW ==========\n");
    SetConsoleColor(CC_RESET);
    wprintf(L"%s\n", script.c_str());

    if (txtModeActive && silentPreviewOnly) {
        SetConsoleColor(CC_YELLOW);
        wprintf(L"\n  Not saved! Exiting.\n");
        SetConsoleColor(CC_RESET);
    } 
    else if (txtModeActive && !autoFilename.empty()) {
        SaveToManualPath(script, autoFilename, autoPath);
    } 
    else {
        SetConsoleColor(CC_YELLOW);
        wprintf(L"  Save to file? (Y/n): ");
        SetConsoleColor(CC_RESET);
        std::wstring saveAns = ReadLine();

        if (saveAns.empty() || saveAns == L"Y" || saveAns == L"y" || saveAns == L"yes") {
            std::wstring methodChoice;
            
            while (true) {
                SetConsoleColor(CC_YELLOW);
                wprintf(L"  Choose Save Method:\n");
                wprintf(L"    1 = Type path manually\n");
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
                    wprintf(L"\n  Dialog canceled. Defaulting to manual path entry...\n");
                    SetConsoleColor(CC_RESET);
                    SaveToManualPath(script);
                }
            }
        } else {
            SetConsoleColor(CC_YELLOW);
            wprintf(L"\n  Not saved! Exiting.\n");
            SetConsoleColor(CC_RESET);
        }
    }

    SetConsoleColor(CC_CYAN);
    wprintf(L"\n  Press Enter to exit...");
    SetConsoleColor(CC_RESET);
    ReadLine();
    return 0;
}