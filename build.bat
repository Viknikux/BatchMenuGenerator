@echo off
setlocal enabledelayedexpansion

REM ============================================================
REM Build BatchMenuGeneratorCLI - Dual Target (x64 and x86)
REM Statically linked MSVCRT - Zero external dependencies
REM ============================================================

set "SRC=BatchMenuGenerator.cpp"
set "OUT_X64=BatchMenuGenerator.exe"
set "OUT_X86=BatchMenuGenerator_x86.exe"
set "RC_SRC=app.rc"
set "RES=app.res"

REM Locate Visual Studio Installation Directory (Supports VS 2026 / v18 and older)
set "VCVARS_DIR="
if defined VS180COMNTOOLS set "VCVARS_DIR=%VS180COMNTOOLS%..\..\VC\Auxiliary\Build"
if not defined VCVARS_DIR if defined VS170COMNTOOLS set "VCVARS_DIR=%VS170COMNTOOLS%..\..\VC\Auxiliary\Build"
if not defined VCVARS_DIR if defined VS160COMNTOOLS set "VCVARS_DIR=%VS160COMNTOOLS%..\..\VC\Auxiliary\Build"
if not defined VCVARS_DIR if defined VS150COMNTOOLS set "VCVARS_DIR=%VS150COMNTOOLS%..\..\VC\Auxiliary\Build"

REM Fallback explicit path check for VS 2026 Community if environment variables aren't caught
if not defined VCVARS_DIR (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VCVARS_DIR=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build"
    )
)

if not defined VCVARS_DIR (
    echo.
    echo ERROR: Visual Studio installation not found!
    echo Please run this script from a Developer Command Prompt.
    echo.
    pause
    exit /b 1
)

REM Common Compiler & Linker Flags
set "CL_FLAGS=/nologo /O2 /Oi /Ot /GL /MT /EHsc /W4 /wd4100 /wd4101 /wd4189 /wd4458 /D _UNICODE /D UNICODE /D WIN32_LEAN_AND_MEAN /D _CRT_SECURE_NO_WARNINGS"
set "LINK_FLAGS=/link /SUBSYSTEM:CONSOLE,6.1 /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO user32.lib gdi32.lib comctl32.lib dwmapi.lib shell32.lib kernel32.lib"

echo ============================================================
echo Building 64-bit Target (%OUT_X64%)
echo ============================================================
echo.

call "%VCVARS_DIR%\vcvars64.bat" >nul 2>&1

REM Compile Resources for x64
rc.exe /nologo /fo"%RES%" "%RC_SRC%"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Resource compilation for x64 failed!
    pause
    exit /b 1
)

REM Compile x64 Binary
cl.exe %CL_FLAGS% %SRC% %RES% /Fe%OUT_X64% %LINK_FLAGS%
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: x64 Build FAILED!
    pause
    exit /b 1
)

echo.
echo === x64 Build SUCCEEDED ===
echo Output: %OUT_X64%
echo.

REM Clean temporary res file before x86 compile
if exist "%RES%" del "%RES%"

echo ============================================================
echo Building 32-bit Target (%OUT_X86%)
echo ============================================================
echo.

call "%VCVARS_DIR%\vcvars32.bat" >nul 2>&1

REM Compile Resources for x86
rc.exe /nologo /fo"%RES%" "%RC_SRC%"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Resource compilation for x86 failed!
    pause
    exit /b 1
)

REM Compile x86 Binary
cl.exe %CL_FLAGS% %SRC% %RES% /Fe%OUT_X86% %LINK_FLAGS%
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: x86 Build FAILED!
    pause
    exit /b 1
)

echo.
echo === x86 Build SUCCEEDED ===
echo Output: %OUT_X86%
echo.

REM Clean up compiled object and resource files
if exist "*.obj" del "*.obj"
if exist "%RES%" del "%RES%"

echo ============================================================
echo ALL BUILDS COMPLETED SUCCESSFULLY
echo ============================================================
dir %OUT_X64% %OUT_X86%
echo.

endlocal