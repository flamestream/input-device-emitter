#pragma once
#include <windows.h>
#include <string>
#include <string_view>

class Console {
private:
    static HANDLE hConsole;
    static WORD originalAttributes;
    static void fillRemainingLineWithSpaces();
public:

    static void saveAttributes();
    static void print(std::string_view line);
    static void log(std::string_view line);
    static void error(std::string_view line);
    static void warn(std::string_view line);
    static void info(std::string_view line);
    static void success(std::string_view line);
    static void eraseLines(const unsigned int count = 1);
};
