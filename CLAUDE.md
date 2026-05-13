# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

CMake-based C++ project using Ninja and MSVC compilers on Windows.

```bash
# Configure and build
cmake --preset x64-debug
cmake --build --preset x64-debug

# Build directly
cmake --build out/build/x64-debug
```

Available presets: `x64-debug`, `x64-release`, `x86-debug`, `x86-release`

Can also open `.slnx` in Visual Studio 2022.

## Architecture

- **Project type**: Win32 API desktop application (C++20)
- **Entry point**: `main.cpp` → `wWinMain()`
- **Build output**: `out/build/<preset>/FileSize.exe`

## Modules

| File | Purpose |
|------|---------|
| `main.cpp` | WinMain, window creation, event loop. Contains all UI logic directly |
| `FileScanner.h/.cpp` | Single-pass recursive folder traversal and size calculation |
| `FileSorter.h/.cpp` | Sort by size (descending) or name |
| `SizeFormatter.h/.cpp` | Bytes → KB/MB/GB conversion |

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
