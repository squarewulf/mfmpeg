#include "mfmpeg.h"

static std::thread       g_encodeThread;
static std::atomic<bool> g_encoding(false);
static std::atomic<bool> g_cancelRequested(false);
static HANDLE            g_ffmpegProcess = nullptr;
static std::mutex        g_processMutex;

static std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string utf8(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    return utf8;
}

static double ParseFPS(const wchar_t* fpsStr) {
    const wchar_t* slash = wcschr(fpsStr, L'/');
    if (slash) {
        double num = _wtof(fpsStr);
        double den = _wtof(slash + 1);
        if (den > 0) return num / den;
    }
    double val = _wtof(fpsStr);
    return val > 0 ? val : 24.0;
}

std::wstring FindFFmpeg() {
    wchar_t exeDir[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    PathRemoveFileSpecW(exeDir);

    std::wstring local = std::wstring(exeDir) + L"\\ffmpeg.exe";
    if (GetFileAttributesW(local.c_str()) != INVALID_FILE_ATTRIBUTES)
        return local;

    wchar_t found[MAX_PATH] = {};
    if (SearchPathW(nullptr, L"ffmpeg.exe", nullptr, MAX_PATH, found, nullptr))
        return found;

    return L"";
}

std::wstring BuildFFmpegCommand(const ImageSequence& seq,
                                const ExportSettings& settings,
                                const std::wstring& outputPath,
                                const std::wstring& ffmpegPath,
                                const std::wstring& fileListPath) {
    std::wostringstream cmd;
    cmd << L"\"" << ffmpegPath << L"\" -y -hide_banner -stats";

    if (!fileListPath.empty()) {
        cmd << L" -f concat -safe 0 -i \"" << fileListPath << L"\"";
    } else {
        cmd << L" -framerate " << FPS_FFMPEG_VALUES[settings.fpsIndex];

        if (seq.frameCount <= 1 && !seq.files.empty()) {
            cmd << L" -i \"" << seq.files[0] << L"\"";
        } else {
            int start = (settings.startFrame > 0) ? settings.startFrame : seq.firstFrame;
            cmd << L" -start_number " << start;
            cmd << L" -i \"" << seq.directory << L"\\" << seq.prefix;
            if (seq.padding > 0)
                cmd << L"%0" << seq.padding << L"d";
            else
                cmd << L"%d";
            cmd << seq.extension << L"\"";
        }

        int startF = (settings.startFrame > 0) ? settings.startFrame : seq.firstFrame;
        int endF   = (settings.endFrame > 0)   ? settings.endFrame   : seq.lastFrame;
        if (endF < startF) endF = startF;
        int numFrames = endF - startF + 1;

        if (numFrames < seq.frameCount && seq.frameCount > 1) {
            cmd << L" -vframes " << numFrames;
        }
    }

    bool isWebM = (settings.formatIndex == 2);
    bool isMOV  = (settings.formatIndex == 1);
    bool preserveAlpha = (settings.alphaIndex == 3 && (isWebM || isMOV));
    bool whiteBG = (settings.alphaIndex == 2);

    if (whiteBG) {
        cmd << L" -filter_complex \"color=white:s="
            << seq.width << L"x" << seq.height
            << L"[bg];[bg][0:v]overlay=shortest=1,format=yuv420p\"";
    } else if (!preserveAlpha) {
        cmd << L" -pix_fmt yuv420p";
    }

    if (preserveAlpha && isMOV) {
        cmd << L" -c:v prores_ks -profile:v 4444 -pix_fmt yuva444p10le";
        cmd << L" -vendor apl0";
        int qScale = 12 - settings.qualityIndex * 2;
        if (qScale < 2) qScale = 2;
        cmd << L" -q:v " << qScale;
    } else if (preserveAlpha && isWebM) {
        cmd << L" -pix_fmt yuva420p";
        cmd << L" -c:v libvpx-vp9 -b:v 0";
        cmd << L" -crf " << QUALITY_VP9_CRF[settings.qualityIndex];
        cmd << L" -row-mt 1";
    } else if (isWebM) {
        cmd << L" -c:v libvpx-vp9 -b:v 0";
        cmd << L" -crf " << QUALITY_VP9_CRF[settings.qualityIndex];
        cmd << L" -row-mt 1";
    } else {
        bool hevc = (settings.codecIndex == 1);
        switch (settings.gpuIndex) {
            case 0:
                cmd << L" -c:v " << (hevc ? L"libx265" : L"libx264");
                cmd << L" -preset " << PRESET_NAMES[settings.presetIndex];
                cmd << L" -crf " << QUALITY_CRF[settings.qualityIndex];
                break;
            case 1:
                cmd << L" -c:v " << (hevc ? L"hevc_nvenc" : L"h264_nvenc");
                cmd << L" -preset p" << NVENC_PRESET_MAP[settings.presetIndex];
                cmd << L" -rc constqp -cq " << QUALITY_CRF[settings.qualityIndex];
                break;
            case 2:
                cmd << L" -c:v " << (hevc ? L"hevc_amf" : L"h264_amf");
                cmd << L" -rc cqp -qp_i " << QUALITY_CRF[settings.qualityIndex];
                cmd << L" -qp_p " << QUALITY_CRF[settings.qualityIndex];
                break;
            case 3:
                cmd << L" -c:v " << (hevc ? L"hevc_qsv" : L"h264_qsv");
                cmd << L" -global_quality " << QUALITY_CRF[settings.qualityIndex];
                break;
        }
    }

    if (isMOV && !preserveAlpha) {
        cmd << L" -movflags +faststart";
    }

    cmd << L" \"" << outputPath << L"\"";
    return cmd.str();
}

static void EncodeThreadFunc(HWND hwndNotify, std::wstring command,
                             int totalFrames, std::wstring tempFileList) {
    g_encoding = true;
    g_cancelRequested = false;

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        wchar_t* msg = new wchar_t[256];
        wcscpy_s(msg, 256, L"Failed to create pipe for FFmpeg.");
        PostMessage(hwndNotify, WM_ENCODE_COMPLETE, 0, (LPARAM)msg);
        g_encoding = false;
        return;
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    std::vector<wchar_t> cmdBuf(command.begin(), command.end());
    cmdBuf.push_back(L'\0');

    BOOL created = CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hWritePipe);

    if (!created) {
        CloseHandle(hReadPipe);
        wchar_t* msg = new wchar_t[256];
        wcscpy_s(msg, 256, L"Failed to start FFmpeg. Make sure ffmpeg.exe is available.");
        PostMessage(hwndNotify, WM_ENCODE_COMPLETE, 0, (LPARAM)msg);
        g_encoding = false;
        if (!tempFileList.empty()) DeleteFileW(tempFileList.c_str());
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_processMutex);
        g_ffmpegProcess = pi.hProcess;
    }

    char buffer[4096];
    std::string lineBuffer;
    DWORD bytesRead;

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        if (g_cancelRequested) break;

        buffer[bytesRead] = '\0';
        lineBuffer += buffer;

        size_t searchPos = 0;
        while (true) {
            size_t pos = lineBuffer.find("frame=", searchPos);
            if (pos == std::string::npos) break;

            size_t numStart = pos + 6;
            while (numStart < lineBuffer.size() && lineBuffer[numStart] == ' ') numStart++;
            size_t numEnd = numStart;
            while (numEnd < lineBuffer.size() && isdigit((unsigned char)lineBuffer[numEnd])) numEnd++;

            if (numEnd > numStart) {
                int currentFrame = atoi(lineBuffer.substr(numStart, numEnd - numStart).c_str());
                PostMessage(hwndNotify, WM_ENCODE_PROGRESS, (WPARAM)currentFrame, (LPARAM)totalFrames);
            }

            size_t lineEnd = lineBuffer.find_first_of("\r\n", numEnd);
            if (lineEnd == std::string::npos) {
                lineBuffer = lineBuffer.substr(pos);
                break;
            }
            searchPos = lineEnd + 1;
        }

        if (lineBuffer.size() > 8192) {
            lineBuffer = lineBuffer.substr(lineBuffer.size() - 1024);
        }
    }

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, 5000);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    {
        std::lock_guard<std::mutex> lock(g_processMutex);
        g_ffmpegProcess = nullptr;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (!tempFileList.empty()) {
        DeleteFileW(tempFileList.c_str());
    }

    wchar_t* msg = new wchar_t[512];
    if (g_cancelRequested) {
        wcscpy_s(msg, 512, L"Export cancelled.");
        PostMessage(hwndNotify, WM_ENCODE_COMPLETE, 0, (LPARAM)msg);
    } else if (exitCode == 0) {
        wcscpy_s(msg, 512, L"Export complete!");
        PostMessage(hwndNotify, WM_ENCODE_COMPLETE, 1, (LPARAM)msg);
    } else {
        swprintf_s(msg, 512, L"FFmpeg error (exit code %lu). Check codec/GPU settings.", exitCode);
        PostMessage(hwndNotify, WM_ENCODE_COMPLETE, 0, (LPARAM)msg);
    }

    g_encoding = false;
}

void StartEncode(HWND hwndNotify,
                 const ImageSequence& seq,
                 const ExportSettings& settings,
                 const std::wstring& outputPath,
                 const std::wstring& ffmpegPath) {
    if (g_encoding) return;

    if (g_encodeThread.joinable()) {
        g_encodeThread.join();
    }

    std::wstring fileListPath;

    if (seq.useFileList && !seq.files.empty()) {
        wchar_t tempDir[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, tempDir);
        fileListPath = std::wstring(tempDir) + L"mfmpeg_v2_filelist.txt";

        double fps = ParseFPS(FPS_FFMPEG_VALUES[settings.fpsIndex]);
        double frameDuration = 1.0 / fps;

        FILE* f = nullptr;
        _wfopen_s(&f, fileListPath.c_str(), L"wb");
        if (f) {
            fprintf(f, "ffconcat version 1.0\n");
            for (size_t i = 0; i < seq.files.size(); ++i) {
                std::string pathUtf8 = WideToUtf8(seq.files[i]);
                std::replace(pathUtf8.begin(), pathUtf8.end(), '\\', '/');

                std::string escaped;
                for (char c : pathUtf8) {
                    if (c == '\'') escaped += "'\\''";
                    else escaped += c;
                }

                fprintf(f, "file '%s'\n", escaped.c_str());
                if (i < seq.files.size() - 1) {
                    fprintf(f, "duration %.8f\n", frameDuration);
                }
            }
            fclose(f);
        }
    }

    std::wstring command = BuildFFmpegCommand(seq, settings, outputPath, ffmpegPath, fileListPath);

    int startF = (settings.startFrame > 0) ? settings.startFrame : seq.firstFrame;
    int endF   = (settings.endFrame > 0)   ? settings.endFrame   : seq.lastFrame;
    int totalFrames = endF - startF + 1;
    if (totalFrames < 1) totalFrames = seq.frameCount;

    g_encodeThread = std::thread(EncodeThreadFunc, hwndNotify, command, totalFrames, fileListPath);
}

void CancelEncode() {
    g_cancelRequested = true;
    std::lock_guard<std::mutex> lock(g_processMutex);
    if (g_ffmpegProcess) {
        TerminateProcess(g_ffmpegProcess, 1);
    }
}

bool IsEncoding() {
    return g_encoding.load();
}
