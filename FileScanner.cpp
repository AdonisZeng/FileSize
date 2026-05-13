#include "FileScanner.h"

FileScanner::FileScanner() : m_directCount(0), m_totalSize(0) {}

std::vector<FileEntry> FileScanner::ScanDirectory(const std::wstring& path) {
    m_entries.clear();
    m_directCount = 0;
    m_totalSize = 0;
    ScanTopLevelEntries(path);
    return m_entries;
}

void FileScanner::EnumerateDirectory(const std::wstring& path, std::function<void(const WIN32_FIND_DATAW&, const std::wstring&)> callback) {
    std::wstring searchPath = path + L"\\*";

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        std::wstring name = findData.cFileName;

        if (name == L"." || name == L"..") {
            continue;
        }

        std::wstring fullPath = path + L"\\" + name;
        callback(findData, fullPath);
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

void FileScanner::ScanTopLevelEntries(const std::wstring& path) {
    EnumerateDirectory(path, [&](const WIN32_FIND_DATAW& findData, const std::wstring& fullPath) {
        std::wstring name = findData.cFileName;
        bool isReparsePoint = (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        bool isFolder = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        unsigned long long size = 0;

        if (isReparsePoint && isFolder) {
            m_directCount++;
            FileEntry entry;
            entry.name = name;
            entry.path = fullPath;
            entry.size = 0;
            entry.isFolder = true;
            entry.isReparsePoint = true;
            m_entries.push_back(entry);
            return;
        }

        if (isFolder) {
            size = ScanAndAccumulate(fullPath);
        } else {
            size = ((unsigned long long)findData.nFileSizeHigh << 32) | findData.nFileSizeLow;
        }

        m_directCount++;
        m_totalSize += size;

        FileEntry entry;
        entry.name = name;
        entry.path = fullPath;
        entry.size = size;
        entry.isFolder = isFolder;
        entry.isReparsePoint = false;
        m_entries.push_back(entry);
    });
}

unsigned long long FileScanner::ScanAndAccumulate(const std::wstring& path) {
    unsigned long long totalSize = 0;

    EnumerateDirectory(path, [&](const WIN32_FIND_DATAW& findData, const std::wstring& fullPath) {
        bool isReparsePoint = (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        bool isFolder = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (isReparsePoint && isFolder) {
            return;
        }

        if (isFolder) {
            totalSize += ScanAndAccumulate(fullPath);
        } else {
            unsigned long long fileSize = ((unsigned long long)findData.nFileSizeHigh << 32) | findData.nFileSizeLow;
            totalSize += fileSize;
        }
    });

    return totalSize;
}
