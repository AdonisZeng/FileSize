#include "FileSorter.h"
#include <algorithm>

void FileSorter::Sort(std::vector<FileEntry>& entries, SortCriteria criteria) {
    switch (criteria) {
    case SortCriteria::BySize:
        std::sort(entries.begin(), entries.end(), CompareBySize);
        break;
    case SortCriteria::ByName:
        std::sort(entries.begin(), entries.end(), CompareByName);
        break;
    }
}

bool FileSorter::CompareBySize(const FileEntry& a, const FileEntry& b) {
    if (a.size != b.size) {
        return a.size > b.size;
    }
    return a.name < b.name;
}

bool FileSorter::CompareByName(const FileEntry& a, const FileEntry& b) {
    if (a.isFolder != b.isFolder) {
        return a.isFolder;
    }
    return a.name < b.name;
}
