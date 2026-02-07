@echo off
setlocal
echo ============================================
echo   MFmpeg v2 Build
echo ============================================

if not exist icon.ico (
    if exist ..\icon.ico (
        copy ..\icon.ico icon.ico >nul
        echo Copied icon.ico from parent directory.
    ) else (
        echo WARNING: icon.ico not found. Build may fail.
    )
)

echo Compiling resources...
rc /nologo /fo resources.res resources.rc
if errorlevel 1 goto :fail

echo Compiling source...
cl /nologo /EHsc /std:c++17 /O2 /W3 /DUNICODE /D_UNICODE ^
    main.cpp sequence.cpp encoder.cpp theme.cpp settings.cpp ^
    resources.res ^
    /Fe:MFmpegV2.exe ^
    /link user32.lib gdi32.lib gdiplus.lib shell32.lib comdlg32.lib ^
    comctl32.lib ole32.lib dwmapi.lib shlwapi.lib uxtheme.lib urlmon.lib
if errorlevel 1 goto :fail

del *.obj 2>nul
echo.
echo ============================================
echo   Build successful: MFmpegV2.exe
echo ============================================
goto :end

:fail
echo.
echo ============================================
echo   BUILD FAILED
echo ============================================

:end
endlocal
