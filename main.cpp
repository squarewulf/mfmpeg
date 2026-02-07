#include "mfmpeg.h"

// ============================================================
//  Globals
// ============================================================

static HINSTANCE g_hInst        = nullptr;
static HWND hwndMain            = nullptr;
static HWND hwndPathEdit        = nullptr;
static HWND hwndBrowse          = nullptr;
static HWND hwndOptionsBtn      = nullptr;
static HWND hwndPreview         = nullptr;
static HWND hwndSlider          = nullptr;
static HWND hwndFrameLabel      = nullptr;
static HWND hwndFrameTotal      = nullptr;
static HWND hwndSeqInfo         = nullptr;
static HWND hwndFPS             = nullptr;
static HWND hwndQuality         = nullptr;
static HWND hwndCodec           = nullptr;
static HWND hwndPreset          = nullptr;
static HWND hwndFormat          = nullptr;
static HWND hwndGPU             = nullptr;
static HWND hwndStartFrame      = nullptr;
static HWND hwndEndFrame        = nullptr;
static HWND hwndProgress        = nullptr;
static HWND hwndExport          = nullptr;
static HWND hwndCancel          = nullptr;
static HWND hwndStatus          = nullptr;

static HWND hwndLblFPS          = nullptr;
static HWND hwndLblQuality      = nullptr;
static HWND hwndLblCodec        = nullptr;
static HWND hwndLblPreset       = nullptr;
static HWND hwndLblFormat       = nullptr;
static HWND hwndLblGPU          = nullptr;
static HWND hwndLblRange        = nullptr;
static HWND hwndLblTo           = nullptr;
static HWND hwndLblAlpha        = nullptr;
static HWND hwndAlpha           = nullptr;

static HWND hwndOptionsWnd      = nullptr;
static HWND hwndOptTheme        = nullptr;
static HWND hwndOptShowPopup    = nullptr;

static HWND hwndDonePopup       = nullptr;

static ImageSequence  g_sequence   = {};
static ExportSettings g_settings   = {};
static int            g_currentFrame = 0;
static std::wstring   g_ffmpegPath;

static Gdiplus::Bitmap* g_previewBitmap = nullptr;
static std::wstring     g_previewPath;

static HBRUSH g_bgBrush      = nullptr;
static HBRUSH g_panelBrush    = nullptr;
static HBRUSH g_controlBrush  = nullptr;
static HFONT  g_font          = nullptr;
static HFONT  g_fontBold      = nullptr;
static ULONG_PTR g_gdiplusToken = 0;

static std::vector<std::wstring> g_collectedFiles;
static std::mutex g_collectMutex;

static int g_sepY1 = 0;
static int g_sepY2 = 0;

enum DragTarget { DRAG_NONE, DRAG_IN, DRAG_OUT, DRAG_PLAYHEAD };
static DragTarget g_dragTarget = DRAG_NONE;
static int g_inFrame  = 0;
static int g_outFrame = 0;
static bool g_syncingMarkers = false;

static const int MIN_W = 540;
static const int MIN_H = 500;
static const int DEFAULT_W = 660;
static const int DEFAULT_H = 700;
static const int MARGIN = 10;

// ============================================================
//  Forward Declarations
// ============================================================

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK OptionsProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK TimelineProc(HWND, UINT, WPARAM, LPARAM);
static void CreateControls(HWND hwnd);
static void LayoutControls(int clientW, int clientH);
static void PopulateComboBoxes();
static void ApplySettingsToControls();
static void ReadSettingsFromControls();
static void LoadSequenceFromFile(const std::wstring& path);
static void LoadSequenceFromFiles(const std::vector<std::wstring>& files);
static void UpdateSequenceUI();
static void UpdatePreviewFrame();
static void UpdateFrameLabel();
static void UpdateSeqInfo();
static void UpdateTitle();
static void SwitchTheme(int index);
static void RecreateThemeBrushes();
static void ApplyDarkModeToControl(HWND hwnd, bool dark);
static void ApplyDarkModeToAllControls(bool dark);
static void DoBrowse();
static void DoExport();
static void ShowOptionsDialog();
static void OnEncodeProgress(int current, int total);
static void OnEncodeComplete(bool success, const wchar_t* message);
static void SetUIEncoding(bool encoding);
static void UpdateStatus(const wchar_t* text);
static void AddCollectedFile(const std::wstring& path);
static void ProcessCollectedFiles();

// ============================================================
//  Helpers
// ============================================================

static HWND MakeLabel(HWND parent, const wchar_t* text,
                      int x, int y, int w, int h, DWORD extra = 0) {
    return CreateWindowW(L"STATIC", text,
        WS_CHILD | WS_VISIBLE | SS_LEFT | extra,
        x, y, w, h, parent, nullptr, g_hInst, nullptr);
}

static HWND MakeCombo(HWND parent, int id, int x, int y, int w) {
    return CreateWindowW(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        x, y, w, 200, parent, (HMENU)(INT_PTR)id, g_hInst, nullptr);
}

static HWND MakeEdit(HWND parent, int id, int x, int y, int w, int h) {
    return CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_BORDER,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, g_hInst, nullptr);
}

static void ComboAdd(HWND combo, const wchar_t* text) {
    SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)text);
}

static void ComboSet(HWND combo, int index) {
    SendMessageW(combo, CB_SETCURSEL, (WPARAM)index, 0);
}

static int ComboGet(HWND combo) {
    return (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
}

static void MoveCtrl(HWND hwnd, int x, int y, int w, int h) {
    if (hwnd) SetWindowPos(hwnd, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

// ============================================================
//  Layout
// ============================================================

static void LayoutControls(int w, int h) {
    const int M = MARGIN;
    const int usableW = w - M * 2;

    // --- Top bar (from top) ---
    int topY = M;
    int gearW = 36;
    int browseW = 80;
    int gap = 5;
    int editX = 50;
    int editW = w - editX - browseW - gearW - gap * 2 - M;

    MoveCtrl(hwndPathEdit, editX, topY, editW, 24);
    MoveCtrl(hwndBrowse, editX + editW + gap, topY - 1, browseW, 26);
    MoveCtrl(hwndOptionsBtn, w - M - gearW, topY - 1, gearW, 26);

    // --- Bottom section (positioned from bottom up) ---
    int btnExportW = 95;
    int btnCancelW = 90;

    int exportY = h - M - 32;
    MoveCtrl(hwndExport, w - M - btnExportW, exportY, btnExportW, 32);
    MoveCtrl(hwndCancel, w - M - btnExportW - gap - btnCancelW, exportY, btnCancelW, 32);
    MoveCtrl(hwndStatus, M + 5, exportY + 6, w - btnExportW - btnCancelW - M * 2 - gap * 2 - 10, 20);

    int progY = exportY - 30;
    MoveCtrl(hwndProgress, M + 5, progY, usableW - 10, 20);

    g_sepY2 = progY - 10;

    // Settings grid: 4 rows, 30px each
    int rowH = 30;
    int labelW = 82;
    int comboW = min(160, (w - M * 2 - 40) / 2 - labelW);
    if (comboW < 100) comboW = 100;

    int col1Lbl = M + 5;
    int col1Ctrl = col1Lbl + labelW + 5;
    int col2Lbl = w / 2 + 5;
    int col2Ctrl = col2Lbl + labelW + 5;
    int col2ComboW = min(comboW, w - col2Ctrl - M - 5);
    if (col2ComboW < 60) col2ComboW = 60;

    int row4Y = g_sepY2 - rowH - 2;
    int row3Y = row4Y - rowH;
    int row2Y = row3Y - rowH;
    int row1Y = row2Y - rowH;

    // Row 1: FPS / Format
    MoveCtrl(hwndLblFPS, col1Lbl, row1Y + 3, labelW, 18);
    MoveCtrl(hwndFPS, col1Ctrl, row1Y, comboW, 200);
    MoveCtrl(hwndLblFormat, col2Lbl, row1Y + 3, labelW, 18);
    MoveCtrl(hwndFormat, col2Ctrl, row1Y, col2ComboW, 200);

    // Row 2: Quality / GPU
    MoveCtrl(hwndLblQuality, col1Lbl, row2Y + 3, labelW, 18);
    MoveCtrl(hwndQuality, col1Ctrl, row2Y, comboW, 200);
    MoveCtrl(hwndLblGPU, col2Lbl, row2Y + 3, labelW, 18);
    MoveCtrl(hwndGPU, col2Ctrl, row2Y, col2ComboW, 200);

    // Row 3: Codec / Range
    MoveCtrl(hwndLblCodec, col1Lbl, row3Y + 3, labelW, 18);
    MoveCtrl(hwndCodec, col1Ctrl, row3Y, comboW, 200);
    MoveCtrl(hwndLblRange, col2Lbl, row3Y + 3, labelW, 18);
    int rangeEditW = 58;
    MoveCtrl(hwndStartFrame, col2Ctrl, row3Y, rangeEditW, 22);
    MoveCtrl(hwndLblTo, col2Ctrl + rangeEditW + 4, row3Y + 3, 16, 18);
    MoveCtrl(hwndEndFrame, col2Ctrl + rangeEditW + 24, row3Y, rangeEditW, 22);

    // Row 4: Preset / Alpha
    MoveCtrl(hwndLblPreset, col1Lbl, row4Y + 3, labelW, 18);
    MoveCtrl(hwndPreset, col1Ctrl, row4Y, comboW, 200);
    MoveCtrl(hwndLblAlpha, col2Lbl, row4Y + 3, labelW, 18);
    MoveCtrl(hwndAlpha, col2Ctrl, row4Y, col2ComboW, 200);

    g_sepY1 = row1Y - 12;

    // Seq info
    int seqY = g_sepY1 - 22;
    MoveCtrl(hwndSeqInfo, M + 5, seqY, usableW, 18);

    // Frame labels
    int frameY = seqY - 22;
    MoveCtrl(hwndFrameLabel, M + 5, frameY, 300, 18);
    MoveCtrl(hwndFrameTotal, w - M - 205, frameY, 200, 18);

    // Timeline slider
    int sliderY = frameY - 32;
    MoveCtrl(hwndSlider, M, sliderY, usableW, 30);

    // --- Preview (fills remaining space) ---
    int previewTop = topY + 34;
    int previewBottom = sliderY - 5;
    int previewH = previewBottom - previewTop;
    if (previewH < 60) previewH = 60;
    MoveCtrl(hwndPreview, M, previewTop, usableW, previewH);
}

// ============================================================
//  UI Creation
// ============================================================

static void CreateControls(HWND hwnd) {
    g_font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_fontBold = CreateFontW(-14, 0, 0, 0, FW_BOLD, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Top bar (positions are placeholders, LayoutControls will fix them)
    MakeLabel(hwnd, L"File:", 15, 13, 35, 20);
    hwndPathEdit = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY | WS_BORDER,
        0, 0, 100, 24, hwnd, (HMENU)IDC_PATH_EDIT, g_hInst, nullptr);
    hwndBrowse = CreateWindowW(L"BUTTON", L"Browse",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 80, 26, hwnd, (HMENU)IDC_BROWSE, g_hInst, nullptr);
    hwndOptionsBtn = CreateWindowW(L"BUTTON", L"\x2699",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 36, 26, hwnd, (HMENU)IDC_OPTIONS, g_hInst, nullptr);

    // Preview
    WNDCLASSW wcPrev = {};
    wcPrev.lpfnWndProc = [](HWND hw, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
        if (msg == WM_PAINT) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hw, &ps);
            RECT rc;
            GetClientRect(hw, &rc);

            const Theme& theme = GetTheme(g_settings.themeIndex);
            HBRUSH bg = CreateSolidBrush(theme.bgPreview);
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            if (g_previewBitmap) {
                Gdiplus::Graphics gfx(hdc);
                gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

                int imgW = (int)g_previewBitmap->GetWidth();
                int imgH = (int)g_previewBitmap->GetHeight();
                int areaW = rc.right - rc.left;
                int areaH = rc.bottom - rc.top;

                float scaleX = (float)areaW / imgW;
                float scaleY = (float)areaH / imgH;
                float scale = (scaleX < scaleY) ? scaleX : scaleY;

                int drawW = (int)(imgW * scale);
                int drawH = (int)(imgH * scale);
                int drawX = (areaW - drawW) / 2;
                int drawY = (areaH - drawH) / 2;

                int alpha = g_settings.alphaIndex;
                if (alpha == 1) {
                    Gdiplus::SolidBrush black(Gdiplus::Color(255, 0, 0, 0));
                    gfx.FillRectangle(&black, drawX, drawY, drawW, drawH);
                } else if (alpha == 2) {
                    Gdiplus::SolidBrush white(Gdiplus::Color(255, 255, 255, 255));
                    gfx.FillRectangle(&white, drawX, drawY, drawW, drawH);
                } else if (alpha == 3) {
                    int checkSize = 8;
                    Gdiplus::Color c1(255, 180, 180, 180);
                    Gdiplus::Color c2(255, 220, 220, 220);
                    Gdiplus::SolidBrush b1(c1), b2(c2);
                    for (int cy = drawY; cy < drawY + drawH; cy += checkSize) {
                        for (int cx = drawX; cx < drawX + drawW; cx += checkSize) {
                            int cw = min(checkSize, drawX + drawW - cx);
                            int ch = min(checkSize, drawY + drawH - cy);
                            bool light = (((cx - drawX) / checkSize) + ((cy - drawY) / checkSize)) % 2 == 0;
                            gfx.FillRectangle(light ? &b2 : &b1, cx, cy, cw, ch);
                        }
                    }
                }

                gfx.DrawImage(g_previewBitmap, drawX, drawY, drawW, drawH);
            } else {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, GetTheme(g_settings.themeIndex).textDim);
                SelectObject(hdc, g_font);
                RECT txtRc = rc;
                DrawTextW(hdc, L"No sequence loaded", -1, &txtRc,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            EndPaint(hw, &ps);
            return 0;
        }
        if (msg == WM_ERASEBKGND) return 1;
        return DefWindowProcW(hw, msg, wp, lp);
    };
    wcPrev.hInstance = g_hInst;
    wcPrev.lpszClassName = L"MFmpegV2Preview";
    wcPrev.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wcPrev);

    hwndPreview = CreateWindowW(L"MFmpegV2Preview", L"",
        WS_CHILD | WS_VISIBLE, 0, 0, 100, 100,
        hwnd, (HMENU)IDC_PREVIEW, g_hInst, nullptr);

    // Timeline (custom slider with in/out markers)
    {
        static bool timelineRegistered = false;
        if (!timelineRegistered) {
            WNDCLASSW wcTL = {};
            wcTL.lpfnWndProc = [](HWND hw, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
                return TimelineProc(hw, msg, wp, lp);
            };
            wcTL.hInstance = g_hInst;
            wcTL.lpszClassName = L"MFmpegV2Timeline";
            wcTL.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            RegisterClassW(&wcTL);
            timelineRegistered = true;
        }
        hwndSlider = CreateWindowW(L"MFmpegV2Timeline", L"",
            WS_CHILD | WS_VISIBLE, 0, 0, 100, 30,
            hwnd, (HMENU)IDC_FRAME_SLIDER, g_hInst, nullptr);
    }

    // Frame info
    hwndFrameLabel = MakeLabel(hwnd, L"No sequence loaded", 0, 0, 300, 18);
    hwndFrameTotal = MakeLabel(hwnd, L"", 0, 0, 200, 18, SS_RIGHT);

    // Seq info
    hwndSeqInfo = MakeLabel(hwnd, L"", 0, 0, 400, 18);

    // Settings labels (stored so LayoutControls can move them)
    hwndLblFPS     = MakeLabel(hwnd, L"Frame Rate", 0, 0, 82, 18);
    hwndLblQuality = MakeLabel(hwnd, L"Quality",    0, 0, 82, 18);
    hwndLblCodec   = MakeLabel(hwnd, L"Codec",      0, 0, 82, 18);
    hwndLblPreset  = MakeLabel(hwnd, L"Preset",     0, 0, 82, 18);
    hwndLblFormat  = MakeLabel(hwnd, L"Format",     0, 0, 82, 18);
    hwndLblGPU     = MakeLabel(hwnd, L"GPU Accel",  0, 0, 82, 18);
    hwndLblRange   = MakeLabel(hwnd, L"Range",      0, 0, 82, 18);
    hwndLblTo      = MakeLabel(hwnd, L"to",         0, 0, 16, 18);
    hwndLblAlpha   = MakeLabel(hwnd, L"Alpha",      0, 0, 82, 18);

    // Settings combos
    hwndFPS    = MakeCombo(hwnd, IDC_FPS,    0, 0, 150);
    hwndQuality= MakeCombo(hwnd, IDC_QUALITY,0, 0, 150);
    hwndCodec  = MakeCombo(hwnd, IDC_CODEC,  0, 0, 150);
    hwndPreset = MakeCombo(hwnd, IDC_PRESET, 0, 0, 150);
    hwndFormat = MakeCombo(hwnd, IDC_FORMAT, 0, 0, 150);
    hwndGPU    = MakeCombo(hwnd, IDC_GPU,    0, 0, 150);
    hwndAlpha  = MakeCombo(hwnd, IDC_ALPHA,  0, 0, 150);

    hwndStartFrame = MakeEdit(hwnd, IDC_START_FRAME, 0, 0, 58, 22);
    hwndEndFrame   = MakeEdit(hwnd, IDC_END_FRAME,   0, 0, 58, 22);

    // Progress
    hwndProgress = CreateWindowW(PROGRESS_CLASSW, L"",
        WS_CHILD | WS_VISIBLE, 0, 0, 100, 20,
        hwnd, (HMENU)IDC_PROGRESS, g_hInst, nullptr);
    SendMessageW(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    // Status + Buttons
    hwndStatus = MakeLabel(hwnd, L"Ready", 0, 0, 300, 20);

    hwndCancel = CreateWindowW(L"BUTTON", L"Cancel",
        WS_CHILD | BS_PUSHBUTTON, 0, 0, 90, 32,
        hwnd, (HMENU)IDC_CANCEL, g_hInst, nullptr);

    hwndExport = CreateWindowW(L"BUTTON", L"\x25B6  Export",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 95, 32,
        hwnd, (HMENU)IDC_EXPORT, g_hInst, nullptr);

    // Apply font to all children
    EnumChildWindows(hwnd, [](HWND child, LPARAM lp) -> BOOL {
        SendMessageW(child, WM_SETFONT, (WPARAM)lp, TRUE);
        return TRUE;
    }, (LPARAM)g_font);

    PopulateComboBoxes();
    ApplySettingsToControls();

    // Initial layout
    RECT rc;
    GetClientRect(hwnd, &rc);
    LayoutControls(rc.right, rc.bottom);
}

// ============================================================
//  ComboBox Population
// ============================================================

static void PopulateComboBoxes() {
    for (int i = 0; i < FPS_COUNT; ++i)     ComboAdd(hwndFPS, FPS_NAMES[i]);
    for (int i = 0; i < QUALITY_COUNT; ++i)  ComboAdd(hwndQuality, QUALITY_NAMES[i]);
    for (int i = 0; i < CODEC_COUNT; ++i)    ComboAdd(hwndCodec, CODEC_NAMES[i]);
    for (int i = 0; i < PRESET_COUNT; ++i)   ComboAdd(hwndPreset, PRESET_NAMES[i]);
    for (int i = 0; i < FORMAT_COUNT; ++i)   ComboAdd(hwndFormat, FORMAT_NAMES[i]);
    for (int i = 0; i < GPU_COUNT; ++i)      ComboAdd(hwndGPU, GPU_NAMES[i]);
    for (int i = 0; i < ALPHA_COUNT; ++i)   ComboAdd(hwndAlpha, ALPHA_NAMES[i]);
}

static void ApplySettingsToControls() {
    ComboSet(hwndFPS,     g_settings.fpsIndex);
    ComboSet(hwndQuality, g_settings.qualityIndex);
    ComboSet(hwndCodec,   g_settings.codecIndex);
    ComboSet(hwndPreset,  g_settings.presetIndex);
    ComboSet(hwndFormat,  g_settings.formatIndex);
    ComboSet(hwndGPU,     g_settings.gpuIndex);
    ComboSet(hwndAlpha,   g_settings.alphaIndex);

    if (g_sequence.valid) {
        wchar_t buf[32];
        swprintf_s(buf, L"%d", g_sequence.firstFrame);
        SetWindowTextW(hwndStartFrame, buf);
        swprintf_s(buf, L"%d", g_sequence.lastFrame);
        SetWindowTextW(hwndEndFrame, buf);
    }
}

static void ReadSettingsFromControls() {
    g_settings.fpsIndex     = ComboGet(hwndFPS);
    g_settings.qualityIndex = ComboGet(hwndQuality);
    g_settings.codecIndex   = ComboGet(hwndCodec);
    g_settings.presetIndex  = ComboGet(hwndPreset);
    g_settings.formatIndex  = ComboGet(hwndFormat);
    g_settings.gpuIndex     = ComboGet(hwndGPU);
    g_settings.alphaIndex   = ComboGet(hwndAlpha);

    wchar_t buf[32];
    GetWindowTextW(hwndStartFrame, buf, 32);
    g_settings.startFrame = _wtoi(buf);
    GetWindowTextW(hwndEndFrame, buf, 32);
    g_settings.endFrame = _wtoi(buf);
}

// ============================================================
//  File Collection (single-instance multi-select)
// ============================================================

static void AddCollectedFile(const std::wstring& path) {
    if (path.empty()) return;
    DWORD attr = GetFileAttributesW(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) return;

    {
        std::lock_guard<std::mutex> lock(g_collectMutex);
        g_collectedFiles.push_back(path);
    }

    KillTimer(hwndMain, TIMER_COLLECT_FILES);
    SetTimer(hwndMain, TIMER_COLLECT_FILES, COLLECT_DELAY_MS, nullptr);
}

static void ProcessCollectedFiles() {
    std::vector<std::wstring> files;
    {
        std::lock_guard<std::mutex> lock(g_collectMutex);
        files = std::move(g_collectedFiles);
        g_collectedFiles.clear();
    }

    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());

    if (files.empty()) return;

    if (files.size() == 1)
        LoadSequenceFromFile(files[0]);
    else
        LoadSequenceFromFiles(files);
}

// ============================================================
//  Sequence Loading
// ============================================================

static void LoadSequenceFromFile(const std::wstring& path) {
    g_sequence = DetectSequence(path);
    if (!g_sequence.valid) {
        UpdateStatus(L"Could not detect image sequence from selected file.");
        SetWindowTextW(hwndPathEdit, path.c_str());
        return;
    }
    SetWindowTextW(hwndPathEdit, path.c_str());
    UpdateSequenceUI();
    UpdateStatus(L"Sequence loaded \x2014 full sequence detected from file.");
}

static void LoadSequenceFromFiles(const std::vector<std::wstring>& files) {
    g_sequence = BuildSequenceFromFiles(files);
    if (!g_sequence.valid) {
        UpdateStatus(L"Could not build sequence from selected files.");
        return;
    }
    wchar_t info[512];
    swprintf_s(info, L"%s  (%d files selected)", g_sequence.directory.c_str(), g_sequence.frameCount);
    SetWindowTextW(hwndPathEdit, info);
    UpdateSequenceUI();

    wchar_t status[128];
    swprintf_s(status, L"Loaded %d selected files as sequence.", g_sequence.frameCount);
    UpdateStatus(status);
}

static void UpdateSequenceUI() {
    g_currentFrame = 0;
    g_inFrame = 0;
    g_outFrame = max(0, g_sequence.frameCount - 1);
    InvalidateRect(hwndSlider, nullptr, FALSE);

    wchar_t buf[32];
    swprintf_s(buf, L"%d", g_sequence.firstFrame);
    SetWindowTextW(hwndStartFrame, buf);
    swprintf_s(buf, L"%d", g_sequence.lastFrame);
    SetWindowTextW(hwndEndFrame, buf);

    UpdatePreviewFrame();
    UpdateFrameLabel();
    UpdateSeqInfo();
    UpdateTitle();
}

// ============================================================
//  Preview
// ============================================================

static void UpdatePreviewFrame() {
    if (!g_sequence.valid || g_sequence.files.empty()) return;

    int idx = g_currentFrame;
    if (idx < 0) idx = 0;
    if (idx >= (int)g_sequence.files.size()) idx = (int)g_sequence.files.size() - 1;

    const std::wstring& framePath = g_sequence.files[idx];
    if (framePath == g_previewPath && g_previewBitmap) return;

    delete g_previewBitmap;
    g_previewBitmap = nullptr;
    g_previewPath.clear();

    g_previewBitmap = Gdiplus::Bitmap::FromFile(framePath.c_str());
    if (g_previewBitmap && g_previewBitmap->GetLastStatus() == Gdiplus::Ok) {
        g_previewPath = framePath;
    } else {
        delete g_previewBitmap;
        g_previewBitmap = nullptr;
    }

    InvalidateRect(hwndPreview, nullptr, FALSE);
}

static void UpdateFrameLabel() {
    if (!g_sequence.valid) {
        SetWindowTextW(hwndFrameLabel, L"No sequence loaded");
        SetWindowTextW(hwndFrameTotal, L"");
        return;
    }
    int displayFrame = g_sequence.firstFrame + g_currentFrame;
    wchar_t buf[128];
    swprintf_s(buf, L"Frame %d", displayFrame);
    SetWindowTextW(hwndFrameLabel, buf);
    swprintf_s(buf, L"%d / %d frames", g_currentFrame + 1, g_sequence.frameCount);
    SetWindowTextW(hwndFrameTotal, buf);
}

static void UpdateSeqInfo() {
    if (!g_sequence.valid) {
        SetWindowTextW(hwndSeqInfo, L"");
        return;
    }
    std::wstring name = g_sequence.prefix;
    while (!name.empty() && (name.back() == L'_' || name.back() == L'-' || name.back() == L'.'))
        name.pop_back();
    if (name.empty()) name = L"(unnamed)";

    wchar_t buf[512];
    if (g_sequence.useFileList) {
        swprintf_s(buf, L"%s  \x2502  %d files (selected)  \x2502  %dx%d  \x2502  %s",
            name.c_str(), g_sequence.frameCount,
            g_sequence.width, g_sequence.height,
            g_sequence.extension.c_str() + 1);
    } else {
        swprintf_s(buf, L"%s  \x2502  %d frames  \x2502  %dx%d  \x2502  %s",
            name.c_str(), g_sequence.frameCount,
            g_sequence.width, g_sequence.height,
            g_sequence.extension.c_str() + 1);
    }
    SetWindowTextW(hwndSeqInfo, buf);
}

static void UpdateTitle() {
    if (!g_sequence.valid) {
        SetWindowTextW(hwndMain, L"MFmpeg v2");
        return;
    }
    std::wstring name = g_sequence.prefix;
    while (!name.empty() && (name.back() == L'_' || name.back() == L'-' || name.back() == L'.'))
        name.pop_back();
    if (name.empty()) name = L"sequence";
    wchar_t buf[256];
    swprintf_s(buf, L"MFmpeg v2 \x2014 %s (%d frames)", name.c_str(), g_sequence.frameCount);
    SetWindowTextW(hwndMain, buf);
}

// ============================================================
//  Theme
// ============================================================

static void RecreateThemeBrushes() {
    if (g_bgBrush)      DeleteObject(g_bgBrush);
    if (g_panelBrush)    DeleteObject(g_panelBrush);
    if (g_controlBrush)  DeleteObject(g_controlBrush);
    const Theme& t = GetTheme(g_settings.themeIndex);
    g_bgBrush      = CreateSolidBrush(t.bgMain);
    g_panelBrush   = CreateSolidBrush(t.bgPanel);
    g_controlBrush = CreateSolidBrush(t.bgControl);
}

static void ApplyDarkModeToControl(HWND hwnd, bool dark) {
    if (dark)
        SetWindowTheme(hwnd, L"DarkMode_CFD", nullptr);
    else
        SetWindowTheme(hwnd, L"", nullptr);
}

static void ApplyDarkModeToAllControls(bool dark) {
    HWND ctrls[] = { hwndFPS, hwndQuality, hwndCodec, hwndPreset,
                     hwndFormat, hwndGPU, hwndAlpha,
                     hwndStartFrame, hwndEndFrame, hwndPathEdit };
    for (HWND h : ctrls) {
        if (h) ApplyDarkModeToControl(h, dark);
    }
}

// ============================================================
//  Custom Timeline Control
// ============================================================

static const int TL_PAD = 10;
static const int TL_TRACK_H = 4;
static const int TL_THUMB_W = 3;
static const int TL_MARKER_W = 8;
static const int TL_HIT_SLOP = 6;

static int FrameToX(int frame, int totalFrames, int trackW) {
    if (totalFrames <= 1) return 0;
    return (int)((double)frame / (totalFrames - 1) * trackW);
}

static int XToFrame(int x, int totalFrames, int trackW) {
    if (totalFrames <= 1 || trackW <= 0) return 0;
    int f = (int)((double)x / trackW * (totalFrames - 1) + 0.5);
    if (f < 0) f = 0;
    if (f >= totalFrames) f = totalFrames - 1;
    return f;
}

static void SyncMarkersToEdits() {
    if (!g_sequence.valid) return;
    g_syncingMarkers = true;
    wchar_t buf[32];
    swprintf_s(buf, L"%d", g_sequence.firstFrame + g_inFrame);
    SetWindowTextW(hwndStartFrame, buf);
    swprintf_s(buf, L"%d", g_sequence.firstFrame + g_outFrame);
    SetWindowTextW(hwndEndFrame, buf);
    g_syncingMarkers = false;
}

static void SyncEditsToMarkers() {
    if (!g_sequence.valid || g_syncingMarkers) return;
    wchar_t buf[32];
    GetWindowTextW(hwndStartFrame, buf, 32);
    int sf = _wtoi(buf) - g_sequence.firstFrame;
    GetWindowTextW(hwndEndFrame, buf, 32);
    int ef = _wtoi(buf) - g_sequence.firstFrame;
    int maxF = g_sequence.frameCount - 1;
    if (sf < 0) sf = 0; if (sf > maxF) sf = maxF;
    if (ef < 0) ef = 0; if (ef > maxF) ef = maxF;
    if (sf > ef) sf = ef;
    g_inFrame = sf;
    g_outFrame = ef;
    InvalidateRect(hwndSlider, nullptr, FALSE);
}

static LRESULT CALLBACK TimelineProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
    RECT rc;
    GetClientRect(hw, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int trackL = TL_PAD;
    int trackR = w - TL_PAD;
    int trackW = trackR - trackL;
    int totalFrames = g_sequence.valid ? g_sequence.frameCount : 1;
    int midY = h / 2;

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hw, &ps);
        const Theme& theme = GetTheme(g_settings.themeIndex);

        HBRUSH bg = CreateSolidBrush(theme.bgMain);
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        int trackTop = midY - TL_TRACK_H / 2;
        int trackBot = midY + TL_TRACK_H / 2;

        RECT trackRc = { trackL, trackTop, trackR, trackBot };
        HBRUSH trackBr = CreateSolidBrush(theme.border);
        FillRect(hdc, &trackRc, trackBr);
        DeleteObject(trackBr);

        if (g_sequence.valid && totalFrames > 1) {
            int inX = trackL + FrameToX(g_inFrame, totalFrames, trackW);
            int outX = trackL + FrameToX(g_outFrame, totalFrames, trackW);

            RECT rangeRc = { inX, trackTop, outX, trackBot };
            HBRUSH rangeBr = CreateSolidBrush(theme.accent);
            FillRect(hdc, &rangeRc, rangeBr);
            DeleteObject(rangeBr);

            POINT inPoly[5] = {
                { inX, midY - 10 },
                { inX + TL_MARKER_W, midY - 10 },
                { inX + TL_MARKER_W, midY + 4 },
                { inX, midY + 10 },
                { inX, midY - 10 }
            };
            HBRUSH inBr = CreateSolidBrush(theme.accent);
            HPEN inPen = CreatePen(PS_SOLID, 1, theme.accent);
            HBRUSH oldBr = (HBRUSH)SelectObject(hdc, inBr);
            HPEN oldPen = (HPEN)SelectObject(hdc, inPen);
            Polygon(hdc, inPoly, 5);
            SelectObject(hdc, oldBr);
            SelectObject(hdc, oldPen);
            DeleteObject(inBr);
            DeleteObject(inPen);

            POINT outPoly[5] = {
                { outX, midY - 10 },
                { outX - TL_MARKER_W, midY - 10 },
                { outX - TL_MARKER_W, midY + 4 },
                { outX, midY + 10 },
                { outX, midY - 10 }
            };
            HBRUSH outBr = CreateSolidBrush(theme.accent);
            HPEN outPen = CreatePen(PS_SOLID, 1, theme.accent);
            oldBr = (HBRUSH)SelectObject(hdc, outBr);
            oldPen = (HPEN)SelectObject(hdc, outPen);
            Polygon(hdc, outPoly, 5);
            SelectObject(hdc, oldBr);
            SelectObject(hdc, oldPen);
            DeleteObject(outBr);
            DeleteObject(outPen);

            int playX = trackL + FrameToX(g_currentFrame, totalFrames, trackW);
            RECT thumbRc = { playX - 1, midY - 12, playX + 2, midY + 12 };
            HBRUSH thumbBr = CreateSolidBrush(theme.accentText);
            FillRect(hdc, &thumbRc, thumbBr);
            DeleteObject(thumbBr);
        }

        EndPaint(hw, &ps);
        return 0;
    }

    case WM_ERASEBKGND: return 1;

    case WM_LBUTTONDOWN: {
        if (!g_sequence.valid || totalFrames <= 1) return 0;
        int mx = (int)(short)LOWORD(lp);
        int inX = trackL + FrameToX(g_inFrame, totalFrames, trackW);
        int outX = trackL + FrameToX(g_outFrame, totalFrames, trackW);
        int playX = trackL + FrameToX(g_currentFrame, totalFrames, trackW);

        if (abs(mx - inX) <= TL_HIT_SLOP + TL_MARKER_W)
            g_dragTarget = DRAG_IN;
        else if (abs(mx - outX) <= TL_HIT_SLOP + TL_MARKER_W)
            g_dragTarget = DRAG_OUT;
        else
            g_dragTarget = DRAG_PLAYHEAD;

        SetCapture(hw);
        SendMessage(hw, WM_MOUSEMOVE, wp, lp);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (g_dragTarget == DRAG_NONE) return 0;
        int mx = (int)(short)LOWORD(lp);
        int frame = XToFrame(mx - trackL, totalFrames, trackW);

        if (g_dragTarget == DRAG_IN) {
            if (frame > g_outFrame) frame = g_outFrame;
            g_inFrame = frame;
            SyncMarkersToEdits();
        } else if (g_dragTarget == DRAG_OUT) {
            if (frame < g_inFrame) frame = g_inFrame;
            g_outFrame = frame;
            SyncMarkersToEdits();
        } else if (g_dragTarget == DRAG_PLAYHEAD) {
            if (frame != g_currentFrame) {
                g_currentFrame = frame;
                UpdatePreviewFrame();
                UpdateFrameLabel();
            }
        }
        InvalidateRect(hw, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
        if (g_dragTarget != DRAG_NONE) {
            g_dragTarget = DRAG_NONE;
            ReleaseCapture();
        }
        return 0;

    case WM_SETCURSOR: {
        if (!g_sequence.valid || totalFrames <= 1) break;
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hw, &pt);
        int mx = pt.x;
        int inX = trackL + FrameToX(g_inFrame, totalFrames, trackW);
        int outX = trackL + FrameToX(g_outFrame, totalFrames, trackW);
        if (abs(mx - inX) <= TL_HIT_SLOP + TL_MARKER_W ||
            abs(mx - outX) <= TL_HIT_SLOP + TL_MARKER_W) {
            SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
            return TRUE;
        }
        break;
    }
    }
    return DefWindowProcW(hw, msg, wp, lp);
}

static void SwitchTheme(int index) {
    g_settings.themeIndex = index;
    const Theme& theme = GetTheme(index);
    RecreateThemeBrushes();
    ApplyThemeToWindow(hwndMain, theme);
    ApplyDarkModeToAllControls(theme.darkMode);
    SendMessageW(hwndProgress, PBM_SETBARCOLOR, 0, (LPARAM)theme.progress);

    InvalidateRect(hwndMain, nullptr, TRUE);
    EnumChildWindows(hwndMain, [](HWND child, LPARAM) -> BOOL {
        InvalidateRect(child, nullptr, TRUE);
        return TRUE;
    }, 0);

    if (hwndOptionsWnd) {
        ApplyThemeToWindow(hwndOptionsWnd, theme);
        if (hwndOptTheme) ApplyDarkModeToControl(hwndOptTheme, theme.darkMode);
        if (hwndOptShowPopup) ApplyDarkModeToControl(hwndOptShowPopup, theme.darkMode);
        InvalidateRect(hwndOptionsWnd, nullptr, TRUE);
        EnumChildWindows(hwndOptionsWnd, [](HWND child, LPARAM) -> BOOL {
            InvalidateRect(child, nullptr, TRUE);
            return TRUE;
        }, 0);
    }

    if (hwndDonePopup) {
        ApplyThemeToWindow(hwndDonePopup, theme);
        InvalidateRect(hwndDonePopup, nullptr, TRUE);
        EnumChildWindows(hwndDonePopup, [](HWND child, LPARAM) -> BOOL {
            InvalidateRect(child, nullptr, TRUE);
            return TRUE;
        }, 0);
    }
}

// ============================================================
//  Options Dialog
// ============================================================

static LRESULT CALLBACK OptionsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT boldFont = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        HWND titleLabel = CreateWindowW(L"STATIC", L"Appearance",
            WS_CHILD | WS_VISIBLE, 20, 15, 200, 22,
            hwnd, nullptr, g_hInst, nullptr);
        SendMessageW(titleLabel, WM_SETFONT, (WPARAM)boldFont, TRUE);

        CreateWindowW(L"STATIC", L"Theme",
            WS_CHILD | WS_VISIBLE, 20, 52, 60, 20,
            hwnd, nullptr, g_hInst, nullptr);

        hwndOptTheme = CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            90, 49, 185, 200,
            hwnd, (HMENU)1001, g_hInst, nullptr);

        for (int i = 0; i < GetThemeCount(); ++i)
            SendMessageW(hwndOptTheme, CB_ADDSTRING, 0, (LPARAM)GetTheme(i).name);
        SendMessageW(hwndOptTheme, CB_SETCURSEL, g_settings.themeIndex, 0);

        hwndOptShowPopup = CreateWindowW(L"BUTTON", L"Show export success popup",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            20, 88, 250, 22,
            hwnd, (HMENU)1002, g_hInst, nullptr);

        CreateWindowW(L"BUTTON", L"Close",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            195, 130, 80, 28,
            hwnd, (HMENU)IDOK, g_hInst, nullptr);

        EnumChildWindows(hwnd, [](HWND child, LPARAM lp) -> BOOL {
            SendMessageW(child, WM_SETFONT, (WPARAM)lp, TRUE);
            return TRUE;
        }, (LPARAM)g_font);

        const Theme& theme = GetTheme(g_settings.themeIndex);
        ApplyThemeToWindow(hwnd, theme);
        ApplyDarkModeToControl(hwndOptTheme, theme.darkMode);
        ApplyDarkModeToControl(hwndOptShowPopup, theme.darkMode);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1001 && HIWORD(wParam) == CBN_SELCHANGE) {
            int idx = (int)SendMessageW(hwndOptTheme, CB_GETCURSEL, 0, 0);
            if (idx >= 0 && idx < GetThemeCount()) SwitchTheme(idx);
        }
        else if (LOWORD(wParam) == 1002 && HIWORD(wParam) == BN_CLICKED) {
            g_settings.showExportDone = !g_settings.showExportDone;
            InvalidateRect(hwndOptShowPopup, nullptr, TRUE);
            SaveSettings(g_settings);
        }
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
        if (dis->CtlID == 1002) {
            const Theme& theme = GetTheme(g_settings.themeIndex);
            HBRUSH bgBr = CreateSolidBrush(theme.bgMain);
            FillRect(dis->hDC, &dis->rcItem, bgBr);
            DeleteObject(bgBr);

            int boxSize = 14;
            int y = (dis->rcItem.bottom - dis->rcItem.top - boxSize) / 2;
            RECT boxRc = { 0, y, boxSize, y + boxSize };
            HBRUSH boxBg = CreateSolidBrush(theme.bgControl);
            FillRect(dis->hDC, &boxRc, boxBg);
            DeleteObject(boxBg);
            HPEN borderPen = CreatePen(PS_SOLID, 1, theme.border);
            HPEN oldP = (HPEN)SelectObject(dis->hDC, borderPen);
            HBRUSH oldBr = (HBRUSH)SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
            Rectangle(dis->hDC, boxRc.left, boxRc.top, boxRc.right, boxRc.bottom);
            SelectObject(dis->hDC, oldBr);
            SelectObject(dis->hDC, oldP);
            DeleteObject(borderPen);

            if (g_settings.showExportDone) {
                HPEN checkPen = CreatePen(PS_SOLID, 2, theme.accent);
                HPEN oldCp = (HPEN)SelectObject(dis->hDC, checkPen);
                MoveToEx(dis->hDC, 3, y + 7, nullptr);
                LineTo(dis->hDC, 6, y + 11);
                LineTo(dis->hDC, 12, y + 3);
                SelectObject(dis->hDC, oldCp);
                DeleteObject(checkPen);
            }

            RECT textRc = dis->rcItem;
            textRc.left = boxSize + 6;
            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, theme.text);
            SelectObject(dis->hDC, g_font);
            DrawTextW(dis->hDC, L"Show export success popup", -1, &textRc,
                DT_SINGLELINE | DT_VCENTER | DT_LEFT);

            if (dis->itemState & ODS_FOCUS) {
                RECT focusRc = textRc;
                focusRc.right = focusRc.left + 200;
                DrawFocusRect(dis->hDC, &focusRc);
            }
            return TRUE;
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        const Theme& theme = GetTheme(g_settings.themeIndex);
        HBRUSH bg = CreateSolidBrush(theme.bgMain);
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);
        HPEN pen = CreatePen(PS_SOLID, 1, theme.border);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, 20, 40, nullptr);
        LineTo(hdc, 260, 40);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND: return 1;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        const Theme& theme = GetTheme(g_settings.themeIndex);
        SetTextColor(hdc, theme.text);
        SetBkColor(hdc, theme.bgMain);
        if (g_bgBrush) return (LRESULT)g_bgBrush;
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        const Theme& theme = GetTheme(g_settings.themeIndex);
        SetTextColor(hdc, theme.text);
        SetBkColor(hdc, theme.bgControl);
        if (g_controlBrush) return (LRESULT)g_controlBrush;
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        hwndOptionsWnd = nullptr;
        hwndOptTheme = nullptr;
        hwndOptShowPopup = nullptr;
        EnableWindow(hwndMain, TRUE);
        SetForegroundWindow(hwndMain);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void ShowOptionsDialog() {
    if (hwndOptionsWnd) {
        SetForegroundWindow(hwndOptionsWnd);
        return;
    }
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = OptionsProc;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = L"MFmpegV2Options";
        wc.hIcon = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON));
        RegisterClassExW(&wc);
        registered = true;
    }
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    RECT rc = { 0, 0, 300, 180 };
    AdjustWindowRect(&rc, style, FALSE);
    RECT mainRc;
    GetWindowRect(hwndMain, &mainRc);
    int x = mainRc.left + ((mainRc.right - mainRc.left) - (rc.right - rc.left)) / 2;
    int y = mainRc.top + ((mainRc.bottom - mainRc.top) - (rc.bottom - rc.top)) / 2;
    hwndOptionsWnd = CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"MFmpegV2Options", L"Options", style, x, y,
        rc.right - rc.left, rc.bottom - rc.top,
        hwndMain, nullptr, g_hInst, nullptr);
    EnableWindow(hwndMain, FALSE);
    ShowWindow(hwndOptionsWnd, SW_SHOW);
}

// ============================================================
//  Browse & Export
// ============================================================

static void DoBrowse() {
    wchar_t filePath[MAX_PATH] = {};
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = hwndMain;
    ofn.lpstrFilter =
        L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff;*.exr;*.tga\0"
        L"All Files\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Select any image from the sequence";
    if (GetOpenFileNameW(&ofn)) LoadSequenceFromFile(filePath);
}

static void DoExport() {
    if (!g_sequence.valid) {
        MessageBoxW(hwndMain, L"No sequence loaded. Browse for an image file first.",
            L"No Sequence", MB_ICONINFORMATION);
        return;
    }
    if (g_ffmpegPath.empty()) {
        MessageBoxW(hwndMain,
            L"FFmpeg not found.\n\nPlace ffmpeg.exe next to MFmpegV2.exe or add it to your PATH.",
            L"FFmpeg Missing", MB_ICONERROR);
        return;
    }
    ReadSettingsFromControls();

    std::wstring seqName = g_sequence.prefix;
    while (!seqName.empty() && (seqName.back() == L'_' || seqName.back() == L'-' || seqName.back() == L'.'))
        seqName.pop_back();
    if (seqName.empty()) seqName = L"output";

    int fmtIdx = g_settings.formatIndex;
    if (fmtIdx < 0 || fmtIdx >= FORMAT_COUNT) fmtIdx = 0;
    std::wstring defaultName = seqName + FORMAT_EXTS[fmtIdx];

    wchar_t savePath[MAX_PATH] = {};
    wcscpy_s(savePath, defaultName.c_str());

    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = hwndMain;
    ofn.lpstrFilter = FORMAT_FILTER;
    ofn.nFilterIndex = fmtIdx + 1;
    ofn.lpstrFile = savePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Export Video";
    ofn.lpstrInitialDir = g_sequence.directory.c_str();
    ofn.lpstrDefExt = FORMAT_EXTS[fmtIdx] + 1;

    if (!GetSaveFileNameW(&ofn)) return;

    SetUIEncoding(true);
    UpdateStatus(L"Encoding...");
    SendMessageW(hwndProgress, PBM_SETPOS, 0, 0);
    StartEncode(hwndMain, g_sequence, g_settings, std::wstring(savePath), g_ffmpegPath);
}

// ============================================================
//  Encoding UI
// ============================================================

static void SetUIEncoding(bool encoding) {
    EnableWindow(hwndExport, !encoding);
    EnableWindow(hwndBrowse, !encoding);
    ShowWindow(hwndCancel, encoding ? SW_SHOW : SW_HIDE);
    InvalidateRect(hwndExport, nullptr, TRUE);
}

static void OnEncodeProgress(int current, int total) {
    if (total <= 0) return;
    int pct = (current * 100) / total;
    if (pct > 100) pct = 100;
    SendMessageW(hwndProgress, PBM_SETPOS, pct, 0);
    wchar_t buf[64];
    swprintf_s(buf, L"Encoding... %d%%  (frame %d / %d)", pct, current, total);
    UpdateStatus(buf);
}

static void ShowThemedDonePopup();

static void OnEncodeComplete(bool success, const wchar_t* message) {
    SetUIEncoding(false);
    SendMessageW(hwndProgress, PBM_SETPOS, success ? 100 : 0, 0);
    UpdateStatus(message);
    if (success && g_settings.showExportDone) ShowThemedDonePopup();
}

// ============================================================
//  Themed "Done" Popup
// ============================================================

static LRESULT CALLBACK DonePopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        const Theme& theme = GetTheme(g_settings.themeIndex);
        ApplyThemeToWindow(hwnd, theme);

        HWND hCheck = CreateWindowW(L"STATIC", L"\x2714",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            0, 18, 260, 36,
            hwnd, (HMENU)2001, g_hInst, nullptr);
        HFONT bigFont = CreateFontW(-28, 0, 0, 0, FW_BOLD, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        SendMessageW(hCheck, WM_SETFONT, (WPARAM)bigFont, TRUE);

        HWND hMsg = CreateWindowW(L"STATIC", L"Video exported successfully!",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            10, 58, 240, 22,
            hwnd, (HMENU)2002, g_hInst, nullptr);
        SendMessageW(hMsg, WM_SETFONT, (WPARAM)g_font, TRUE);

        HWND hBtn = CreateWindowW(L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            90, 96, 80, 28,
            hwnd, (HMENU)IDOK, g_hInst, nullptr);
        SendMessageW(hBtn, WM_SETFONT, (WPARAM)g_font, TRUE);

        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        const Theme& theme = GetTheme(g_settings.themeIndex);
        HBRUSH bg = CreateSolidBrush(theme.bgMain);
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);
        HPEN pen = CreatePen(PS_SOLID, 1, theme.accent);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, 0, 0, nullptr); LineTo(hdc, rc.right, 0);
        MoveToEx(hdc, 0, 0, nullptr); LineTo(hdc, 0, rc.bottom);
        MoveToEx(hdc, rc.right - 1, 0, nullptr); LineTo(hdc, rc.right - 1, rc.bottom);
        MoveToEx(hdc, 0, rc.bottom - 1, nullptr); LineTo(hdc, rc.right, rc.bottom - 1);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND: return 1;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        const Theme& theme = GetTheme(g_settings.themeIndex);
        int id = GetDlgCtrlID(hCtrl);
        SetTextColor(hdc, id == 2001 ? theme.accent : theme.text);
        SetBkColor(hdc, theme.bgMain);
        if (g_bgBrush) return (LRESULT)g_bgBrush;
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        hwndDonePopup = nullptr;
        EnableWindow(hwndMain, TRUE);
        SetForegroundWindow(hwndMain);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void ShowThemedDonePopup() {
    if (hwndDonePopup) { DestroyWindow(hwndDonePopup); hwndDonePopup = nullptr; }

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = DonePopupProc;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = L"MFmpegV2Done";
        wc.hIcon = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_APPICON));
        RegisterClassExW(&wc);
        registered = true;
    }

    DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    RECT rc = { 0, 0, 260, 140 };
    AdjustWindowRect(&rc, style, FALSE);
    RECT mainRc;
    GetWindowRect(hwndMain, &mainRc);
    int x = mainRc.left + ((mainRc.right - mainRc.left) - (rc.right - rc.left)) / 2;
    int y = mainRc.top + ((mainRc.bottom - mainRc.top) - (rc.bottom - rc.top)) / 2;

    hwndDonePopup = CreateWindowExW(0,
        L"MFmpegV2Done", L"Done", style, x, y,
        rc.right - rc.left, rc.bottom - rc.top,
        hwndMain, nullptr, g_hInst, nullptr);
    EnableWindow(hwndMain, FALSE);
    ShowWindow(hwndDonePopup, SW_SHOW);
}

static void UpdateStatus(const wchar_t* text) {
    SetWindowTextW(hwndStatus, text);
}

// ============================================================
//  Window Procedure
// ============================================================

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE:
        CreateControls(hwnd);
        return 0;

    case WM_SIZE: {
        if (wParam == SIZE_MINIMIZED) break;
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (w > 0 && h > 0) {
            LayoutControls(w, h);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        RECT rc = { 0, 0, MIN_W, MIN_H };
        DWORD style = (DWORD)GetWindowLongW(hwnd, GWL_STYLE);
        AdjustWindowRect(&rc, style, FALSE);
        mmi->ptMinTrackSize.x = rc.right - rc.left;
        mmi->ptMinTrackSize.y = rc.bottom - rc.top;
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        const Theme& theme = GetTheme(g_settings.themeIndex);
        HBRUSH bg = CreateSolidBrush(theme.bgMain);
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        if (g_sepY1 > 0 && g_sepY2 > 0) {
            HPEN pen = CreatePen(PS_SOLID, 1, theme.border);
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, MARGIN, g_sepY1, nullptr);
            LineTo(hdc, rc.right - MARGIN, g_sepY1);
            MoveToEx(hdc, MARGIN, g_sepY2, nullptr);
            LineTo(hdc, rc.right - MARGIN, g_sepY2);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        const Theme& theme = GetTheme(g_settings.themeIndex);
        SetTextColor(hdc, theme.text);
        SetBkColor(hdc, theme.bgMain);
        if (!g_bgBrush) g_bgBrush = CreateSolidBrush(theme.bgMain);
        return (LRESULT)g_bgBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        const Theme& theme = GetTheme(g_settings.themeIndex);
        SetTextColor(hdc, theme.text);
        SetBkColor(hdc, theme.bgControl);
        if (!g_controlBrush) g_controlBrush = CreateSolidBrush(theme.bgControl);
        return (LRESULT)g_controlBrush;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        const Theme& theme = GetTheme(g_settings.themeIndex);
        SetTextColor(hdc, theme.text);
        SetBkColor(hdc, theme.bgControl);
        if (!g_controlBrush) g_controlBrush = CreateSolidBrush(theme.bgControl);
        return (LRESULT)g_controlBrush;
    }

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
        if (dis->CtlID == IDC_EXPORT) {
            const Theme& theme = GetTheme(g_settings.themeIndex);

            HBRUSH parentBg = CreateSolidBrush(theme.bgMain);
            FillRect(dis->hDC, &dis->rcItem, parentBg);
            DeleteObject(parentBg);

            COLORREF btnColor = (dis->itemState & ODS_DISABLED)
                ? theme.border
                : (dis->itemState & ODS_SELECTED) ? theme.accentHover : theme.accent;

            HBRUSH fill = CreateSolidBrush(btnColor);
            HPEN pen = CreatePen(PS_SOLID, 1, btnColor);
            HBRUSH oldBrush = (HBRUSH)SelectObject(dis->hDC, fill);
            HPEN oldPen = (HPEN)SelectObject(dis->hDC, pen);
            RoundRect(dis->hDC, dis->rcItem.left, dis->rcItem.top,
                      dis->rcItem.right, dis->rcItem.bottom, 6, 6);
            SelectObject(dis->hDC, oldBrush);
            SelectObject(dis->hDC, oldPen);
            DeleteObject(fill);
            DeleteObject(pen);

            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, theme.accentText);
            SelectObject(dis->hDC, g_fontBold);
            DrawTextW(dis->hDC, L"\x25B6  Export", -1, &dis->rcItem,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            return TRUE;
        }
        break;
    }

    case WM_KEYDOWN: {
        HWND focused = GetFocus();
        bool inEdit = (focused == hwndStartFrame || focused == hwndEndFrame || focused == hwndPathEdit);
        if (!inEdit && g_sequence.valid && g_sequence.frameCount > 1) {
            int maxF = g_sequence.frameCount - 1;
            if (wParam == 'I') {
                g_inFrame = g_currentFrame;
                if (g_inFrame > g_outFrame) g_outFrame = g_inFrame;
                SyncMarkersToEdits();
                InvalidateRect(hwndSlider, nullptr, FALSE);
                return 0;
            }
            if (wParam == 'O') {
                g_outFrame = g_currentFrame;
                if (g_outFrame < g_inFrame) g_inFrame = g_outFrame;
                SyncMarkersToEdits();
                InvalidateRect(hwndSlider, nullptr, FALSE);
                return 0;
            }
            if (wParam == VK_LEFT) {
                if (g_currentFrame > 0) {
                    g_currentFrame--;
                    UpdatePreviewFrame();
                    UpdateFrameLabel();
                    InvalidateRect(hwndSlider, nullptr, FALSE);
                }
                return 0;
            }
            if (wParam == VK_RIGHT) {
                if (g_currentFrame < maxF) {
                    g_currentFrame++;
                    UpdatePreviewFrame();
                    UpdateFrameLabel();
                    InvalidateRect(hwndSlider, nullptr, FALSE);
                }
                return 0;
            }
            if (wParam == VK_HOME) {
                g_currentFrame = 0;
                UpdatePreviewFrame();
                UpdateFrameLabel();
                InvalidateRect(hwndSlider, nullptr, FALSE);
                return 0;
            }
            if (wParam == VK_END) {
                g_currentFrame = maxF;
                UpdatePreviewFrame();
                UpdateFrameLabel();
                InvalidateRect(hwndSlider, nullptr, FALSE);
                return 0;
            }
        }
        break;
    }

    case WM_HSCROLL:
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == IDC_BROWSE && code == BN_CLICKED) DoBrowse();
        else if (id == IDC_EXPORT && code == BN_CLICKED) DoExport();
        else if (id == IDC_CANCEL && code == BN_CLICKED) {
            CancelEncode();
            UpdateStatus(L"Cancelling...");
        }
        else if (id == IDC_OPTIONS && code == BN_CLICKED) ShowOptionsDialog();
        else if (id == IDC_FORMAT && code == CBN_SELCHANGE) {
            int fmt = ComboGet(hwndFormat);
            bool isWebM = (fmt == 2);
            bool isMOV  = (fmt == 1);
            bool isMP4  = (fmt == 0);
            bool preserve = (ComboGet(hwndAlpha) == 3);
            EnableWindow(hwndGPU, !isWebM && !(isMOV && preserve));
            EnableWindow(hwndCodec, !isWebM && !(isMOV && preserve));
            EnableWindow(hwndPreset, !isWebM && !(isMOV && preserve));
            if (isWebM) ComboSet(hwndGPU, 0);
            if (isMP4 && preserve) {
                ComboSet(hwndAlpha, 0);
                g_settings.alphaIndex = 0;
                InvalidateRect(hwndPreview, nullptr, FALSE);
            }
        }
        else if (id == IDC_ALPHA && code == CBN_SELCHANGE) {
            g_settings.alphaIndex = ComboGet(hwndAlpha);
            int fmt = ComboGet(hwndFormat);
            if (g_settings.alphaIndex == 3 && fmt == 0) {
                ComboSet(hwndFormat, 1);
                fmt = 1;
            }
            bool preserve = (g_settings.alphaIndex == 3);
            bool isWebM = (fmt == 2);
            bool isMOV  = (fmt == 1);
            EnableWindow(hwndGPU, !isWebM && !(isMOV && preserve));
            EnableWindow(hwndCodec, !isWebM && !(isMOV && preserve));
            EnableWindow(hwndPreset, !isWebM && !(isMOV && preserve));
            if (isWebM || (isMOV && preserve)) ComboSet(hwndGPU, 0);
            InvalidateRect(hwndPreview, nullptr, FALSE);
        }
        else if ((id == IDC_START_FRAME || id == IDC_END_FRAME) && code == EN_CHANGE) {
            if (g_dragTarget == DRAG_NONE) SyncEditsToMarkers();
        }
        return 0;
    }

    case WM_TIMER:
        if (wParam == TIMER_COLLECT_FILES) {
            KillTimer(hwnd, TIMER_COLLECT_FILES);
            ProcessCollectedFiles();
        }
        return 0;

    case WM_COPYDATA: {
        COPYDATASTRUCT* cds = (COPYDATASTRUCT*)lParam;
        if (cds && cds->dwData == COPYDATA_FILEPATH && cds->lpData && cds->cbData > 0) {
            std::wstring path((const wchar_t*)cds->lpData);
            AddCollectedFile(path);
        }
        return TRUE;
    }

    case WM_ENCODE_PROGRESS:
        OnEncodeProgress((int)wParam, (int)lParam);
        return 0;

    case WM_ENCODE_COMPLETE: {
        bool success = (wParam != 0);
        wchar_t* message = (wchar_t*)lParam;
        OnEncodeComplete(success, message ? message : L"");
        delete[] message;
        return 0;
    }

    case WM_CLOSE:
        if (IsEncoding()) {
            int r = MessageBoxW(hwnd, L"Encoding in progress. Cancel and exit?",
                L"Confirm Exit", MB_YESNO | MB_ICONWARNING);
            if (r != IDYES) return 0;
            CancelEncode();
            Sleep(500);
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        ReadSettingsFromControls();
        SaveSettings(g_settings);
        delete g_previewBitmap;
        g_previewBitmap = nullptr;
        if (g_bgBrush)     DeleteObject(g_bgBrush);
        if (g_panelBrush)  DeleteObject(g_panelBrush);
        if (g_controlBrush) DeleteObject(g_controlBrush);
        if (g_font)        DeleteObject(g_font);
        if (g_fontBold)    DeleteObject(g_fontBold);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================
//  Entry Point
// ============================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;

    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"MFmpegV2_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(L"MFmpegV2Window", nullptr);
        if (existing && lpCmdLine && lpCmdLine[0]) {
            std::wstring path = lpCmdLine;
            if (path.size() >= 2 && path.front() == L'"' && path.back() == L'"')
                path = path.substr(1, path.size() - 2);
            if (!path.empty()) {
                COPYDATASTRUCT cds = {};
                cds.dwData = COPYDATA_FILEPATH;
                cds.cbData = (DWORD)((path.size() + 1) * sizeof(wchar_t));
                cds.lpData = (PVOID)path.c_str();
                SendMessageW(existing, WM_COPYDATA, 0, (LPARAM)&cds);
            }
            SetForegroundWindow(existing);
            if (IsIconic(existing)) ShowWindow(existing, SW_RESTORE);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    INITCOMMONCONTROLSEX icex = { sizeof(icex),
        ICC_BAR_CLASSES | ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icex);

    Gdiplus::GdiplusStartupInput gdipInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdipInput, nullptr);

    LoadSettings(g_settings);
    g_ffmpegPath = FindFFmpeg();

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"MFmpegV2Window";
    wc.hIconSm = wc.hIcon;
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rc = { 0, 0, DEFAULT_W, DEFAULT_H };
    AdjustWindowRect(&rc, style, FALSE);

    hwndMain = CreateWindowExW(0, L"MFmpegV2Window", L"MFmpeg v2",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwndMain) {
        MessageBoxW(nullptr, L"Failed to create window.", L"Error", MB_ICONERROR);
        return 1;
    }

    RecreateThemeBrushes();
    const Theme& theme = GetTheme(g_settings.themeIndex);
    ApplyThemeToWindow(hwndMain, theme);
    ApplyDarkModeToAllControls(theme.darkMode);
    SendMessageW(hwndProgress, PBM_SETBARCOLOR, 0, (LPARAM)theme.progress);

    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    if (lpCmdLine && lpCmdLine[0]) {
        std::wstring cmdFile = lpCmdLine;
        if (cmdFile.size() >= 2 && cmdFile.front() == L'"' && cmdFile.back() == L'"')
            cmdFile = cmdFile.substr(1, cmdFile.size() - 2);
        if (!cmdFile.empty()) {
            DWORD attr = GetFileAttributesW(cmdFile.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                AddCollectedFile(cmdFile);
            } else if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
                WIN32_FIND_DATAW fd;
                std::wstring search = cmdFile + L"\\*";
                HANDLE hf = FindFirstFileW(search.c_str(), &fd);
                if (hf != INVALID_HANDLE_VALUE) {
                    do {
                        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                        std::wstring name = fd.cFileName;
                        size_t dot = name.rfind(L'.');
                        if (dot != std::wstring::npos && IsImageExtension(name.substr(dot))) {
                            AddCollectedFile(cmdFile + L"\\" + name);
                            break;
                        }
                    } while (FindNextFileW(hf, &fd));
                    FindClose(hf);
                }
            }
        }
    }

    if (g_ffmpegPath.empty()) {
        int r = MessageBoxW(hwndMain,
            L"FFmpeg is required but was not found.\n\n"
            L"Would you like to open the FFmpeg download page?\n"
            L"(Download the \"essentials\" build and place ffmpeg.exe\n"
            L"next to MFmpegV2.exe or in your system PATH)\n\n"
            L"Click Yes to open the download page.\n"
            L"Click No to continue without FFmpeg (export will not work).",
            L"FFmpeg Not Found", MB_YESNO | MB_ICONWARNING);
        if (r == IDYES) {
            ShellExecuteW(nullptr, L"open",
                L"https://www.gyan.dev/ffmpeg/builds/", nullptr, nullptr, SW_SHOWNORMAL);
        }
        UpdateStatus(L"FFmpeg not found -- place ffmpeg.exe next to MFmpegV2.exe or add to PATH.");
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (hwndOptionsWnd && IsDialogMessageW(hwndOptionsWnd, &msg))
            continue;
        if (hwndDonePopup && IsDialogMessageW(hwndDonePopup, &msg))
            continue;

        if (msg.message == WM_KEYDOWN) {
            HWND focused = GetFocus();
            wchar_t cls[32] = {};
            if (focused) GetClassNameW(focused, cls, 32);
            bool isEdit = (_wcsicmp(cls, L"Edit") == 0);
            if (!isEdit && (msg.wParam == 'I' || msg.wParam == 'O' ||
                msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT ||
                msg.wParam == VK_HOME || msg.wParam == VK_END)) {
                SendMessageW(hwndMain, WM_KEYDOWN, msg.wParam, msg.lParam);
                continue;
            }
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_gdiplusToken) Gdiplus::GdiplusShutdown(g_gdiplusToken);
    CoUninitialize();
    return (int)msg.wParam;
}
