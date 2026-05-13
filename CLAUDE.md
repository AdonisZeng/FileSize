# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

CMake-based C++ project using MinGW Makefiles on Windows.

```bash
# Configure and build (run inside build/ directory)
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make

# Build output: build/FileSize.exe
```

Available presets: `x64-debug`, `x64-release`, `x86-debug`, `x86-release`

## Architecture

- **Project type**: Win32 API desktop application (C++20)
- **Entry point**: `src/main.cpp` → `wWinMain()`
- **Build output**: `build/FileSize.exe`

## Directory Structure

```
FileSize/
├── src/           # Source files (.cpp)
├── include/       # Header files (.h)
├── res/           # Resource files (app.manifest)
├── build/         # Build output
├── CMakeLists.txt
└── CLAUDE.md
```

## Modules

| File | Purpose |
|------|---------|
| `src/main.cpp` | WinMain, window creation, event loop. Contains all UI logic directly |
| `include/FileScanner.h` / `src/FileScanner.cpp` | Single-pass recursive folder traversal and size calculation |
| `include/FileSorter.h` / `src/FileSorter.cpp` | Sort by size (descending) or name |
| `include/SizeFormatter.h` / `src/SizeFormatter.cpp` | Bytes → KB/MB/GB conversion |

## UI Layout

```
┌─────────────────────────────────────────────────────────────┐
│  [选择文件夹]  C:\Path\...                      [刷新]     │
├────────────┬────────────────────────────────────────────────┤
│  TreeView  │  ListView (名称 | 大小 | 类型)                 │
│  (250px)   │  Sorted by size descending                     │
└────────────┴────────────────────────────────────────────────┘
```

- TreeView lazy-loads subfolders on expand (TVN_ITEMEXPANDING)
- TreeItemData structs freed on window destroy to prevent memory leaks

## Dependencies

- Windows SDK (shell32.lib, comctl32.lib)
- COM (IFileDialog for folder picker)
