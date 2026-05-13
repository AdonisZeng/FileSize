#pragma once

#include <string>

class SizeFormatter {
public:
    static std::wstring Format(unsigned long long bytes);
    static std::string FormatA(unsigned long long bytes);  // ANSI version
};
