#include "mfmpeg.h"

static const wchar_t* IMAGE_EXTENSIONS[] = {
    L".png", L".jpg", L".jpeg", L".bmp",
    L".tif", L".tiff", L".exr", L".tga"
};
static const int IMAGE_EXT_COUNT = sizeof(IMAGE_EXTENSIONS) / sizeof(IMAGE_EXTENSIONS[0]);

bool IsImageExtension(const std::wstring& ext) {
    for (int i = 0; i < IMAGE_EXT_COUNT; ++i) {
        if (_wcsicmp(ext.c_str(), IMAGE_EXTENSIONS[i]) == 0)
            return true;
    }
    return false;
}

struct ParsedFilename {
    std::wstring prefix;
    std::wstring numericPart;
    std::wstring extension;
    int frameNumber;
    int padding;
    bool hasNumber;
};

static ParsedFilename ParseImageFilename(const std::wstring& filename) {
    ParsedFilename result = {};

    size_t dotPos = filename.rfind(L'.');
    if (dotPos == std::wstring::npos || dotPos == 0) {
        result.prefix = filename;
        return result;
    }

    result.extension = filename.substr(dotPos);
    std::wstring stem = filename.substr(0, dotPos);

    int pos = (int)stem.size() - 1;
    while (pos >= 0 && iswdigit(stem[pos])) {
        --pos;
    }

    int numStart = pos + 1;
    int numLen = (int)stem.size() - numStart;

    if (numLen == 0) {
        result.prefix = stem;
        result.hasNumber = false;
        return result;
    }

    result.prefix = stem.substr(0, numStart);
    result.numericPart = stem.substr(numStart);
    result.frameNumber = _wtoi(result.numericPart.c_str());
    result.padding = numLen;
    result.hasNumber = true;
    return result;
}

ImageSequence DetectSequence(const std::wstring& filePath) {
    ImageSequence seq = {};

    wchar_t dirBuf[MAX_PATH] = {};
    wchar_t fileBuf[MAX_PATH] = {};
    wcscpy_s(dirBuf, filePath.c_str());
    PathRemoveFileSpecW(dirBuf);
    wcscpy_s(fileBuf, PathFindFileNameW(filePath.c_str()));

    seq.directory = dirBuf;
    std::wstring clickedName = fileBuf;

    ParsedFilename parsed = ParseImageFilename(clickedName);
    if (!IsImageExtension(parsed.extension)) {
        return seq;
    }

    if (!parsed.hasNumber) {
        seq.prefix = parsed.prefix;
        seq.extension = parsed.extension;
        seq.padding = 0;
        seq.firstFrame = 0;
        seq.lastFrame = 0;
        seq.frameCount = 1;
        seq.files.push_back(filePath);
        seq.valid = true;
        return seq;
    }

    seq.prefix = parsed.prefix;
    seq.extension = parsed.extension;

    std::wstring searchPattern = seq.directory + L"\\*" + seq.extension;
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return seq;
    }

    struct FrameEntry {
        int number;
        std::wstring fullPath;
    };
    std::vector<FrameEntry> frames;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        std::wstring name = fd.cFileName;
        ParsedFilename pf = ParseImageFilename(name);

        if (!pf.hasNumber) continue;
        if (_wcsicmp(pf.prefix.c_str(), seq.prefix.c_str()) != 0) continue;
        if (_wcsicmp(pf.extension.c_str(), seq.extension.c_str()) != 0) continue;

        std::wstring full = seq.directory + L"\\" + name;
        frames.push_back({ pf.frameNumber, full });

    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    if (frames.empty()) {
        return seq;
    }

    std::sort(frames.begin(), frames.end(),
        [](const FrameEntry& a, const FrameEntry& b) { return a.number < b.number; });

    seq.firstFrame = frames.front().number;
    seq.lastFrame = frames.back().number;
    seq.frameCount = (int)frames.size();
    seq.padding = parsed.padding;

    for (const auto& f : frames) {
        seq.files.push_back(f.fullPath);
    }

    if (!seq.files.empty()) {
        Gdiplus::Bitmap bmp(seq.files[0].c_str());
        if (bmp.GetLastStatus() == Gdiplus::Ok) {
            seq.width = (int)bmp.GetWidth();
            seq.height = (int)bmp.GetHeight();
        }
    }

    seq.valid = true;
    return seq;
}

ImageSequence BuildSequenceFromFiles(const std::vector<std::wstring>& files) {
    ImageSequence seq = {};
    if (files.empty()) return seq;

    wchar_t dirBuf[MAX_PATH] = {};
    wcscpy_s(dirBuf, files[0].c_str());
    PathRemoveFileSpecW(dirBuf);
    seq.directory = dirBuf;

    struct Entry {
        std::wstring path;
        int frameNumber;
        int padding;
        bool hasNumber;
    };
    std::vector<Entry> entries;

    std::wstring commonPrefix;
    std::wstring commonExt;
    bool first = true;

    for (const auto& f : files) {
        DWORD attr = GetFileAttributesW(f.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
            continue;

        std::wstring name = PathFindFileNameW(f.c_str());
        ParsedFilename pf = ParseImageFilename(name);
        if (!IsImageExtension(pf.extension)) continue;

        entries.push_back({ f, pf.frameNumber, pf.padding, pf.hasNumber });

        if (first) {
            commonPrefix = pf.prefix;
            commonExt = pf.extension;
            seq.padding = pf.padding;
            first = false;
        }
    }

    if (entries.empty()) return seq;

    seq.prefix = commonPrefix;
    seq.extension = commonExt;

    bool allHaveNumbers = true;
    for (const auto& e : entries) {
        if (!e.hasNumber) { allHaveNumbers = false; break; }
    }

    if (allHaveNumbers) {
        std::sort(entries.begin(), entries.end(),
            [](const Entry& a, const Entry& b) { return a.frameNumber < b.frameNumber; });
        seq.firstFrame = entries.front().frameNumber;
        seq.lastFrame = entries.back().frameNumber;
    } else {
        std::sort(entries.begin(), entries.end(),
            [](const Entry& a, const Entry& b) { return a.path < b.path; });
        seq.firstFrame = 0;
        seq.lastFrame = (int)entries.size() - 1;
    }

    seq.frameCount = (int)entries.size();
    for (const auto& e : entries)
        seq.files.push_back(e.path);

    bool sequential = allHaveNumbers;
    if (sequential) {
        for (size_t i = 1; i < entries.size(); ++i) {
            if (entries[i].frameNumber != entries[i - 1].frameNumber + 1) {
                sequential = false;
                break;
            }
        }
    }
    seq.useFileList = !sequential;

    if (!seq.files.empty()) {
        Gdiplus::Bitmap bmp(seq.files[0].c_str());
        if (bmp.GetLastStatus() == Gdiplus::Ok) {
            seq.width = (int)bmp.GetWidth();
            seq.height = (int)bmp.GetHeight();
        }
    }

    seq.valid = true;
    return seq;
}
