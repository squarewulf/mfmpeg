# MFmpeg

A lightweight Windows tool for converting image sequences to video. Right-click any image file in a sequence, select **"Open sequence in MFmpeg"**, adjust settings, and export.

![MFmpeg Screenshot](screenshots/preview.png)

## Features

- **One-click workflow** — Right-click an image in a sequence, open in MFmpeg, export as video
- **Smart sequence detection** — Select one file to detect the full sequence, or select specific files to export a subset
- **Single-instance multi-select** — Selecting multiple files opens one MFmpeg window, not dozens
- **Interactive timeline** — Scrub frames with a visual playhead, drag in/out markers, or press **I** / **O** to set range
- **Arrow key navigation** — Left/Right step one frame, Home/End jump to first/last
- **Alpha channel support** — Ignore, composite over black/white, or preserve alpha (ProRes 4444 for MOV, VP9 for WebM)
- **Live alpha preview** — Preview window shows transparency handling in real-time (checkerboard, black, or white background)
- **GPU acceleration** — NVIDIA NVENC, AMD AMF, Intel QSV, or CPU encoding
- **Codec options** — H.264, H.265/HEVC, VP9, ProRes 4444
- **Output formats** — MP4, MOV, WebM
- **Quality & preset control** — CRF-based quality from Lowest to Highest, encoding presets from ultrafast to veryslow
- **Resizable UI** — Works on any screen resolution (540×500 minimum)
- **Themes** — Dark, Light, Retro, Midnight with full DWM integration (Windows 11 Mica, rounded corners)
- **Persistent settings** — All preferences saved between sessions
- **Format support** — PNG, JPG, JPEG, BMP, TIF, TIFF, EXR, TGA

## Requirements

- **Windows 10** or later (x64)
- **FFmpeg** — Place `ffmpeg.exe` next to `MFmpegV2.exe` or add it to your system PATH
  - Download from [gyan.dev](https://www.gyan.dev/ffmpeg/builds/) (get the "essentials" build)
- **MSVC** (for building from source) — Visual Studio 2019+ with C++ Desktop workload

## Quick Start

1. Download the latest release or build from source
2. Place `ffmpeg.exe` next to the executable (or install via PATH)
3. Run the installer or use `install_menu.bat` to add the context menu
4. Right-click any image file → **"Open sequence in MFmpeg"**
5. Adjust settings and click **Export**

## Building from Source

### Prerequisites

- Visual Studio 2019 or later with the **C++ Desktop Development** workload
- Windows SDK 10.0+

### Build

Open a **Developer Command Prompt** (or x64 Native Tools Command Prompt) and run:

```
build.bat
```

This compiles all source files and produces `MFmpegV2.exe`.

### Build Installer

Requires [Inno Setup 6](https://jrsoftware.org/isdl.php):

```
build_installer.bat
```

Output: `installer/MFmpegV2_Setup_2.0.0.exe`

To bundle FFmpeg with the installer, place `ffmpeg.exe` in the project directory before running `build_installer.bat`.

## Context Menu Installation

The installer registers the context menu automatically. For manual installation:

```
install_menu.bat      :: adds "Open sequence in MFmpeg" to image file right-click menus
uninstall_menu.bat    :: removes it
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| **I** | Set in-point at current frame |
| **O** | Set out-point at current frame |
| **←** | Previous frame |
| **→** | Next frame |
| **Home** | Jump to first frame |
| **End** | Jump to last frame |

## Project Structure

```
mfmpeg.h            Master header (includes, structs, constants)
resource.h           Resource IDs
resources.rc         Icon and manifest resource script
app.manifest         Common Controls v6 + DPI awareness
main.cpp             UI, window procedure, timeline control
encoder.cpp          FFmpeg command builder and threaded encoding
sequence.cpp         Image sequence detection and parsing
theme.cpp            Theme definitions and DWM integration
settings.cpp         INI-based settings persistence
build.bat            MSVC build script
build_installer.bat  Inno Setup installer builder
install_menu.bat     Manual context menu registration
uninstall_menu.bat   Manual context menu removal
MFmpegV2_Installer.iss  Inno Setup installer script
```

## License

[MIT](LICENSE)
