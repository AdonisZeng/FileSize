#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <functional>

struct FileEntry {
    std::wstring name;
    std::wstring path;
    unsigned long long size;
    bool isFolder;
    bool isReparsePoint = false;
};

class FileScanner {
public:
    FileScanner();

    std::vector<FileEntry> ScanDirectory(const std::wstring& path);
    unsigned long long GetTotalSize() const { return m_totalSize; }
    size_t GetDirectCount() const { return m_directCount; }

private:
    void ScanTopLevelEntries(const std::wstring& path);
    unsigned long long ScanAndAccumulate(const std::wstring& path);
    static void EnumerateDirectory(const std::wstring& path, std::function<void(const WIN32_FIND_DATAW&, const std::wstring&)> callback);

    std::vector<FileEntry> m_entries;
    size_t m_directCount;
    unsigned long long m_totalSize;
};
