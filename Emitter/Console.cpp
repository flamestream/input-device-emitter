#include "Console.h"
#include <windows.h>
#include <string>
#include <iostream>

HANDLE Console::hConsole = 0;
WORD Console::originalState = 0;

void Console::saveState() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    originalState = consoleInfo.wAttributes;
}

void Console::log(std::string line) {
    std::cout << line << std::endl;
}

void Console::error(std::string line) {
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    log(line);
    SetConsoleTextAttribute(hConsole, originalState);
}

void Console::warn(std::string line) {
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
    log(line);
    SetConsoleTextAttribute(hConsole, originalState);
}

void Console::info(std::string line) {
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN);
    log(line);
    SetConsoleTextAttribute(hConsole, originalState);
}