#pragma once
#include <windows.h>
#include <string>

class Console {
private:
    static HANDLE hConsole;
    static WORD originalAttributes;
    static void fillRemainingLineWithSpaces();
public:

    static void saveAttributes();
    static void print(std::string line);
    static void log(std::string line);
    static void error(std::string line);
    static void warn(std::string line);
    static void info(std::string line);
    static void success(std::string line);
    static void eraseLines(const unsigned int count = 1);
};
