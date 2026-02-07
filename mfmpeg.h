#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <gdiplus.h>
#include <urlmon.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "urlmon.lib")

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// DWM constants (Windows 11)
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

// Custom window messages
#define WM_ENCODE_PROGRESS  (WM_APP + 1)
#define WM_ENCODE_COMPLETE  (WM_APP + 2)

// File collection timer
#define TIMER_COLLECT_FILES  1
#define COLLECT_DELAY_MS     500

// WM_COPYDATA identifier
#define COPYDATA_FILEPATH  0x4D46

// ============================================================
//  Theme
// ============================================================

struct Theme {
    const wchar_t* name;
    COLORREF bgMain;
    COLORREF bgPanel;
    COLORREF bgControl;
    COLORREF bgPreview;
    COLORREF border;
    COLORREF text;
    COLORREF textDim;
    COLORREF accent;
    COLORREF accentHover;
    COLORREF accentText;
    COLORREF progress;
    bool darkMode;
};

int         GetThemeCount();
const Theme& GetTheme(int index);
void        ApplyThemeToWindow(HWND hwnd, const Theme& theme);

// ============================================================
//  Image Sequence
// ============================================================

struct ImageSequence {
    std::wstring directory;
    std::wstring prefix;
    std::wstring separator;
    std::wstring extension;
    int          padding      = 0;
    int          firstFrame   = 0;
    int          lastFrame    = 0;
    int          frameCount   = 0;
    int          width        = 0;
    int          height       = 0;
    std::vector<std::wstring> files;
    bool         valid        = false;
    bool         useFileList  = false;
};

ImageSequence DetectSequence(const std::wstring& filePath);
ImageSequence BuildSequenceFromFiles(const std::vector<std::wstring>& files);
bool          IsImageExtension(const std::wstring& ext);

// ============================================================
//  Export Settings
// ============================================================

struct ExportSettings {
    int fpsIndex        = 3;   // default 24fps
    int qualityIndex    = 2;   // default Medium
    int codecIndex      = 0;   // default H.264
    int presetIndex     = 5;   // default medium
    int formatIndex     = 0;   // default MP4
    int gpuIndex        = 0;   // default None (CPU)
    int alphaIndex      = 0;   // default Ignore
    int startFrame      = 0;   // 0 = use sequence first
    int endFrame        = 0;   // 0 = use sequence last
    int themeIndex      = 0;   // default Dark
    bool showExportDone = true;
};

std::wstring GetSettingsPath();
void         LoadSettings(ExportSettings& settings);
void         SaveSettings(const ExportSettings& settings);

// ============================================================
//  Encoder
// ============================================================

std::wstring FindFFmpeg();
std::wstring BuildFFmpegCommand(const ImageSequence& seq,
                                const ExportSettings& settings,
                                const std::wstring& outputPath,
                                const std::wstring& ffmpegPath,
                                const std::wstring& fileListPath = L"");
void         StartEncode(HWND hwndNotify,
                         const ImageSequence& seq,
                         const ExportSettings& settings,
                         const std::wstring& outputPath,
                         const std::wstring& ffmpegPath);
void         CancelEncode();
bool         IsEncoding();

// ============================================================
//  Constants
// ============================================================

static const wchar_t* QUALITY_NAMES[] = {
    L"Lowest", L"Low", L"Medium", L"High", L"Highest"
};
static const int QUALITY_CRF[] = { 35, 28, 23, 18, 14 };
static const int QUALITY_VP9_CRF[] = { 45, 38, 32, 26, 20 };
static const int QUALITY_COUNT = 5;

static const wchar_t* PRESET_NAMES[] = {
    L"ultrafast", L"superfast", L"veryfast", L"faster", L"fast",
    L"medium", L"slow", L"slower", L"veryslow"
};
static const int PRESET_COUNT = 9;

static const wchar_t* CODEC_NAMES[] = { L"H.264", L"H.265 (HEVC)" };
static const int CODEC_COUNT = 2;

static const wchar_t* FORMAT_NAMES[] = { L"MP4", L"MOV", L"WebM" };
static const wchar_t* FORMAT_EXTS[]  = { L".mp4", L".mov", L".webm" };
static const wchar_t* FORMAT_FILTER =
    L"MP4 Video (*.mp4)\0*.mp4\0"
    L"MOV Video (*.mov)\0*.mov\0"
    L"WebM Video (*.webm)\0*.webm\0"
    L"All Files (*.*)\0*.*\0";
static const int FORMAT_COUNT = 3;

static const wchar_t* GPU_NAMES[] = {
    L"None (CPU)", L"NVIDIA NVENC", L"AMD AMF", L"Intel QSV"
};
static const int GPU_COUNT = 4;

static const wchar_t* FPS_NAMES[] = {
    L"12", L"15", L"23.976", L"24", L"25", L"29.97",
    L"30", L"48", L"50", L"59.94", L"60", L"120"
};
static const wchar_t* FPS_FFMPEG_VALUES[] = {
    L"12", L"15", L"24000/1001", L"24", L"25", L"30000/1001",
    L"30", L"48", L"50", L"60000/1001", L"60", L"120"
};
static const int FPS_COUNT = 12;

static const int NVENC_PRESET_MAP[] = { 1, 1, 2, 3, 4, 5, 5, 6, 7 };

static const wchar_t* ALPHA_NAMES[] = {
    L"Ignore", L"Black BG", L"White BG", L"Preserve"
};
static const int ALPHA_COUNT = 4;
