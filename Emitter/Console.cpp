#include "Console.h"
#include <windows.h>
#include <string>
#include <iostream>

HANDLE Console::hConsole = 0;
WORD Console::originalAttributes = 0;

void Console::saveAttributes() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    originalAttributes = consoleInfo.wAttributes;
}

void Console::print(std::string line) {
    std::cout << line;
    fillRemainingLineWithSpaces(line.size());
    std::cout << std::endl;
}

void Console::log(std::string line) {
    SetConsoleTextAttribute(hConsole, originalAttributes);
    print(line);
}

void Console::error(std::string line) {
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    print(line);
    SetConsoleTextAttribute(hConsole, originalAttributes);
}

void Console::warn(std::string line) {
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
    print(line);
    SetConsoleTextAttribute(hConsole, originalAttributes);
}

void Console::info(std::string line) {
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN);
    print(line);
    SetConsoleTextAttribute(hConsole, originalAttributes); 
}

void Console::success(std::string line) {
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    print(line);
    SetConsoleTextAttribute(hConsole, originalAttributes);
}

void Console::eraseLines(const unsigned int count) {
    if (count == 0)
        return;

    std::cout << "\x1b[2K"; // Delete current line
    for (int i = 1; i < count; ++i)
    {
        std::cout << "\x1b[1A" // Move cursor one line up
            << "\x1b[2K"; // Delete current line
    }
    std::cout << '\r';
}

void Console::fillRemainingLineWithSpaces(int plannedCharacterCount) {
    // Get the console handle
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Get the current cursor position
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int cursorPos = csbi.dwCursorPosition.X;

    // Get the width of the console screen
    int consoleWidth = csbi.dwSize.X;

    // Calculate the number of spaces needed
    int spacesNeeded = consoleWidth - cursorPos - plannedCharacterCount - 1;

    // Output the spaces
    for (int i = 0; i < spacesNeeded; ++i) {
        std::cout.put(' ');
    }
}
