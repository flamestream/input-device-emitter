#pragma once
#include <windows.h>
#include <string>

class Console {
private:
    static HANDLE hConsole;
    static WORD originalState;
public:

    static void saveState();
    static void log(std::string line);
    static void error(std::string line);
    static void warn(std::string line);
    static void info(std::string line);
};
