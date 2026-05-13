#pragma once

#include <vector>
#include "FileScanner.h"

enum class SortCriteria {
    BySize,      // Largest first
    ByName       // Alphabetical A-Z
};

class FileSorter {
public:
    static void Sort(std::vector<FileEntry>& entries, SortCriteria criteria);

    // Comparator functions
    static bool CompareBySize(const FileEntry& a, const FileEntry& b);
    static bool CompareByName(const FileEntry& a, const FileEntry& b);
};
