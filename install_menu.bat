@echo off
setlocal

set "APP=%~dp0MFmpegV2.exe"

if not exist "%APP%" (
    echo ERROR: MFmpegV2.exe not found. Build the project first.
    pause
    exit /b 1
)

echo Installing "Open sequence in MFmpeg" context menu...

for %%E in (.png .jpg .jpeg .bmp .tif .tiff .exr) do (
    reg add "HKCU\Software\Classes\SystemFileAssociations\%%E\shell\MFmpeg" /ve /d "Open sequence in MFmpeg" /f >nul
    reg add "HKCU\Software\Classes\SystemFileAssociations\%%E\shell\MFmpeg" /v "Icon" /d "\"%APP%\"" /f >nul
    reg add "HKCU\Software\Classes\SystemFileAssociations\%%E\shell\MFmpeg\command" /ve /d "\"%APP%\" \"%%1\"" /f >nul
)

echo.
echo Context menu installed for: PNG, JPG, JPEG, BMP, TIF, TIFF, EXR
echo Right-click any image file and select "Open sequence in MFmpeg"
echo.
pause
