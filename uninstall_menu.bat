@echo off
echo Removing "Open sequence in MFmpeg" context menu...

for %%E in (.png .jpg .jpeg .bmp .tif .tiff .exr) do (
    reg delete "HKCU\Software\Classes\SystemFileAssociations\%%E\shell\MFmpeg" /f >nul 2>nul
)

echo.
echo Context menu removed.
echo.
pause
