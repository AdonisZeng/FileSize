#include "SizeFormatter.h"
#include <algorithm>
#include <cstdio>

namespace {

wchar_t* FormatSizeImpl(unsigned long long bytes, const wchar_t* units[], wchar_t* buffer, size_t bufSize) {
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    if (unitIndex == 0) {
        swprintf_s(buffer, bufSize, L"%llu %s", bytes, units[unitIndex]);
    } else {
        swprintf_s(buffer, bufSize, L"%.1f %s", size, units[unitIndex]);
    }
    return buffer;
}

char* FormatSizeImpl(unsigned long long bytes, const char* units[], char* buffer, size_t bufSize) {
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    if (unitIndex == 0) {
        sprintf_s(buffer, bufSize, "%llu %s", bytes, units[unitIndex]);
    } else {
        sprintf_s(buffer, bufSize, "%.1f %s", size, units[unitIndex]);
    }
    return buffer;
}

} // namespace

std::wstring SizeFormatter::Format(unsigned long long bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    wchar_t buffer[64];
    FormatSizeImpl(bytes, units, buffer, _countof(buffer));
    return std::wstring(buffer);
}

std::string SizeFormatter::FormatA(unsigned long long bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    char buffer[64];
    FormatSizeImpl(bytes, units, buffer, _countof(buffer));
    return std::string(buffer);
}
