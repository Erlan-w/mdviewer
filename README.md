# Markdown Viewer

Minimalist, fullscreen Markdown viewer built with C++ and Qt6/Qt5.  
No WebView engine — pure Qt widget rendering.

## Features

- **Full Markdown** — headings, bold/italic/bold-italic, inline code, code blocks, tables, blockquotes, lists, images, horizontal rules
- **Syntax highlighting** — Python, C++, JavaScript/TypeScript
- **Charts** — Bar, Line, Pie via QtCharts (click chart → fullscreen + zoom + drag)
- **Image rendering** — local and remote images (click → fullscreen zoom/drag)
- **Auto TOC** — generated from headings, with hamburger toggle animation
- **File watcher** — auto-reload on save
- **Drag & drop** + **File open dialog**
- **Dark theme** — consistent across all widgets
- **Secure** — stack protector, RELRO, no external scripting engine

## Requirements

| Platform | Requirement |
|----------|-------------|
| Linux    | Qt 6.2+ (or Qt 5.15) + CMake 3.16+ |
| Windows  | Qt 6.5+ MSVC2019/MinGW + CMake 3.16+ |

## Build — Local

```bash
# Clone
git clone https://github.com/yourname/mdviewer.git
cd mdviewer

# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --parallel

# Run
./build/MarkdownViewer demo.md
```

### Linux (Qt6 via apt — Ubuntu 22.04+)

```bash
sudo apt-get install -y \
  qt6-base-dev qt6-charts-dev libqt6svg6-dev \
  libqt6printsupport6 cmake ninja-build
```

### Linux (Qt5 fallback)

```bash
sudo apt-get install -y \
  qtbase5-dev libqt5charts5-dev libqt5svg5-dev \
  qtbase5-private-dev cmake
```

### Windows (vcpkg)

```powershell
vcpkg install qt6-base qt6-charts qt6-svg --triplet x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

## Build — GitHub Actions CI/CD

Push to `main`/`master` → CI builds Linux + Windows automatically.  
Tag a release (`git tag v1.0.0 && git push --tags`) → artifacts attached to GitHub Release.

Workflow file: `.github/workflows/build.yml`

## Usage

```
MarkdownViewer [file.md]
```

| Action              | Shortcut      |
|---------------------|---------------|
| Open file           | Ctrl+O        |
| Reload              | F5 / Ctrl+R   |
| Toggle TOC          | Ctrl+\        |
| Close image/chart   | Escape        |
| Zoom image/chart    | Scroll wheel  |
| Drag image/chart    | Click+drag    |

## Chart Syntax

Embed charts using fenced code blocks with `chart` language:

````markdown
```chart
type:bar          # bar | line | pie
title:My Chart
labels:A,B,C,D
data:10,20,15,30
legend:Series 1
```
````

## Project Structure

```
mdviewer/
├── src/
│   ├── main.cpp              # Entry point
│   ├── MainWindow.{h,cpp}    # Main window + header + drag&drop
│   ├── MarkdownParser.{h,cpp} # AST parser
│   ├── MarkdownRenderer.{h,cpp} # Widget-based renderer
│   ├── TocWidget.{h,cpp}     # Animated TOC sidebar
│   ├── ChartRenderer.{h,cpp} # QtCharts integration
│   ├── ImageViewer.{h,cpp}   # Fullscreen zoomable image
│   └── ThemeManager.h        # Centralized dark theme
├── resources/
│   └── resources.qrc
├── .github/workflows/
│   └── build.yml             # CI/CD
├── CMakeLists.txt
└── demo.md
```
