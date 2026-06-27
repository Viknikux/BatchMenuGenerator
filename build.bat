@echo off
setlocal

REM ============================================================
REM Build BatchMenuGenerator - Pure Win32 C++ Application
REM Statically linked MSVCRT - Zero external dependencies
REM ============================================================

set "SRC=BatchMenuGenerator.cpp"
set "OUT=BatchMenuGenerator.exe"
set "RC_SRC=app.rc"
set "RES=app.res"

REM Check for Visual Studio
if not defined VSINSTALLDIR (
    if defined VS170COMNTOOLS (
        call "%VS170COMNTOOLS%..\..\VC\Auxiliary\Build\vcvars64.bat" 2>nul
    ) else if defined VS160COMNTOOLS (
        call "%VS160COMNTOOLS%..\..\VC\Auxiliary\Build\vcvars64.bat" 2>nul
    ) else if defined VS150COMNTOOLS (
        call "%VS150COMNTOOLS%..\..\VC\Auxiliary\Build\vcvars64.bat" 2>nul
    ) else if defined VS140COMNTOOLS (
        call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" x64 2>nul
    ) else (
        echo.
        echo ERROR: Visual Studio not found!
        echo Please open a "Developer Command Prompt for VS" and run this script.
        echo.
        pause
        exit /b 1
    )
)

echo.
echo === Compiling Resources ===
echo.

REM Compile the resource script into a binary resource file
rc.exe /nologo /fo"%RES%" "%RC_SRC%"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Resource compilation failed!
    pause
    exit /b 1
)

echo.
echo === Compiling BatchMenuGenerator ===
echo.

REM [FIXED] Added %RES% right alongside %SRC% so the compiler links the icon resource object
cl.exe /nologo /O2 /Oi /Ot /GL /MT /EHsc /W4 /wd4100 /wd4101 /wd4189 /wd4458 ^
    /D _UNICODE /D UNICODE /D WIN32_LEAN_AND_MEAN /D _CRT_SECURE_NO_WARNINGS ^
    %SRC% %RES% /Fe%OUT% /link /SUBSYSTEM:WINDOWS /LTCG ^
    /OPT:REF /OPT:ICF /INCREMENTAL:NO ^
    user32.lib gdi32.lib comctl32.lib dwmapi.lib shell32.lib kernel32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo === Build SUCCEEDED ===
    echo Output: %OUT%
    echo.
    dir %OUT%
) else (
    echo.
    echo === Build FAILED ===
    echo.
    pause
    exit /b 1
)

endlocal