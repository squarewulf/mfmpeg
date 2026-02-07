@echo off
setlocal
echo ============================================
echo   MFmpeg v2 Installer Build
echo ============================================
echo.

if not exist MFmpegV2.exe (
    echo ERROR: MFmpegV2.exe not found.
    echo        Run build.bat first to compile the application.
    echo.
    pause
    exit /b 1
)

if not exist icon.ico (
    if exist ..\icon.ico (
        copy ..\icon.ico icon.ico >nul
        echo Copied icon.ico from parent directory.
    ) else (
        echo ERROR: icon.ico not found.
        pause
        exit /b 1
    )
)

if not exist ffmpeg.exe (
    echo WARNING: ffmpeg.exe not found in this directory.
    echo          The installer will work, but the app will prompt
    echo          users to download FFmpeg on first run.
    echo.
    echo          To bundle FFmpeg, download it from:
    echo            https://www.gyan.dev/ffmpeg/builds/
    echo            (get the "essentials" build, extract ffmpeg.exe here)
    echo.
    pause
)

where /q iscc
if errorlevel 1 (
    if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" (
        set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
    ) else if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" (
        set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
    ) else (
        echo ERROR: Inno Setup 6 not found.
        echo        Download from: https://jrsoftware.org/isdl.php
        echo.
        pause
        exit /b 1
    )
) else (
    set "ISCC=iscc"
)

echo Building installer...
echo.
"%ISCC%" MFmpegV2_Installer.iss
if errorlevel 1 (
    echo.
    echo ============================================
    echo   INSTALLER BUILD FAILED
    echo ============================================
    pause
    exit /b 1
)

echo.
echo ============================================
echo   Installer created: installer\MFmpegV2_Setup_2.0.0.exe
echo ============================================
echo.
pause
