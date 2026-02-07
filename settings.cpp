#include "mfmpeg.h"

static std::wstring g_settingsPath;

static void EnsureDirectory(const std::wstring& path) {
    CreateDirectoryW(path.c_str(), nullptr);
}

std::wstring GetSettingsPath() {
    if (!g_settingsPath.empty()) return g_settingsPath;

    wchar_t appData[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
        std::wstring dir = std::wstring(appData) + L"\\MFmpeg";
        EnsureDirectory(dir);
        g_settingsPath = dir + L"\\v2_settings.ini";
    } else {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        PathRemoveFileSpecW(exePath);
        g_settingsPath = std::wstring(exePath) + L"\\v2_settings.ini";
    }
    return g_settingsPath;
}

void LoadSettings(ExportSettings& s) {
    std::wstring path = GetSettingsPath();
    const wchar_t* sec = L"settings";

    s.fpsIndex     = GetPrivateProfileIntW(sec, L"fps",     3, path.c_str());
    s.qualityIndex = GetPrivateProfileIntW(sec, L"quality", 2, path.c_str());
    s.codecIndex   = GetPrivateProfileIntW(sec, L"codec",   0, path.c_str());
    s.presetIndex  = GetPrivateProfileIntW(sec, L"preset",  5, path.c_str());
    s.formatIndex  = GetPrivateProfileIntW(sec, L"format",  0, path.c_str());
    s.gpuIndex     = GetPrivateProfileIntW(sec, L"gpu",     0, path.c_str());
    s.alphaIndex   = GetPrivateProfileIntW(sec, L"alpha",   0, path.c_str());
    s.themeIndex   = GetPrivateProfileIntW(sec, L"theme",   0, path.c_str());
    s.showExportDone = GetPrivateProfileIntW(sec, L"showExportDone", 1, path.c_str()) != 0;

    if (s.fpsIndex < 0 || s.fpsIndex >= FPS_COUNT)         s.fpsIndex = 3;
    if (s.qualityIndex < 0 || s.qualityIndex >= QUALITY_COUNT) s.qualityIndex = 2;
    if (s.codecIndex < 0 || s.codecIndex >= CODEC_COUNT)   s.codecIndex = 0;
    if (s.presetIndex < 0 || s.presetIndex >= PRESET_COUNT) s.presetIndex = 5;
    if (s.formatIndex < 0 || s.formatIndex >= FORMAT_COUNT) s.formatIndex = 0;
    if (s.gpuIndex < 0 || s.gpuIndex >= GPU_COUNT)         s.gpuIndex = 0;
    if (s.alphaIndex < 0 || s.alphaIndex >= ALPHA_COUNT)   s.alphaIndex = 0;
    if (s.themeIndex < 0 || s.themeIndex >= GetThemeCount()) s.themeIndex = 0;
}

void SaveSettings(const ExportSettings& s) {
    std::wstring path = GetSettingsPath();
    const wchar_t* sec = L"settings";

    auto writeInt = [&](const wchar_t* key, int val) {
        wchar_t buf[32];
        swprintf_s(buf, L"%d", val);
        WritePrivateProfileStringW(sec, key, buf, path.c_str());
    };

    writeInt(L"fps",     s.fpsIndex);
    writeInt(L"quality", s.qualityIndex);
    writeInt(L"codec",   s.codecIndex);
    writeInt(L"preset",  s.presetIndex);
    writeInt(L"format",  s.formatIndex);
    writeInt(L"gpu",     s.gpuIndex);
    writeInt(L"alpha",   s.alphaIndex);
    writeInt(L"theme",   s.themeIndex);
    writeInt(L"showExportDone", s.showExportDone ? 1 : 0);
}
