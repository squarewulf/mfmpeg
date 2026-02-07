#include "mfmpeg.h"

static const Theme g_themes[] = {
    {
        L"Dark",
        RGB(18, 18, 24),        // bgMain
        RGB(28, 28, 38),        // bgPanel
        RGB(40, 40, 55),        // bgControl
        RGB(10, 10, 14),        // bgPreview
        RGB(50, 50, 65),        // border
        RGB(230, 230, 235),     // text
        RGB(140, 140, 160),     // textDim
        RGB(0, 120, 212),       // accent
        RGB(30, 145, 235),      // accentHover
        RGB(255, 255, 255),     // accentText
        RGB(0, 180, 80),        // progress
        true                    // darkMode
    },
    {
        L"Light",
        RGB(240, 240, 245),
        RGB(255, 255, 255),
        RGB(255, 255, 255),
        RGB(225, 225, 230),
        RGB(200, 200, 210),
        RGB(30, 30, 30),
        RGB(100, 100, 110),
        RGB(0, 100, 200),
        RGB(20, 120, 220),
        RGB(255, 255, 255),
        RGB(0, 160, 70),
        false
    },
    {
        L"Retro",
        RGB(192, 192, 192),
        RGB(192, 192, 192),
        RGB(255, 255, 255),
        RGB(0, 0, 0),
        RGB(128, 128, 128),
        RGB(0, 0, 0),
        RGB(80, 80, 80),
        RGB(0, 0, 128),
        RGB(0, 0, 180),
        RGB(255, 255, 255),
        RGB(0, 0, 128),
        false
    },
    {
        L"Midnight",
        RGB(15, 10, 30),
        RGB(25, 18, 50),
        RGB(38, 28, 68),
        RGB(8, 5, 18),
        RGB(60, 45, 100),
        RGB(200, 200, 230),
        RGB(120, 110, 160),
        RGB(130, 80, 230),
        RGB(155, 105, 255),
        RGB(255, 255, 255),
        RGB(130, 80, 230),
        true
    }
};

static const int THEME_COUNT = sizeof(g_themes) / sizeof(g_themes[0]);

int GetThemeCount() {
    return THEME_COUNT;
}

const Theme& GetTheme(int index) {
    if (index < 0 || index >= THEME_COUNT) index = 0;
    return g_themes[index];
}

void ApplyThemeToWindow(HWND hwnd, const Theme& theme) {
    BOOL dark = theme.darkMode ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    DWORD corner = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    DWORD backdrop = 2; // DWMSBT_MAINWINDOW (Mica)
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
}
