#include <windows.h>
#include <comdef.h>
#include <string>
#include <atlstr.h>
#include <memory>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "Utility.h"


std::string Utility::wstr2str(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], sizeNeeded, NULL, NULL);
    return str;
}

std::string Utility::hr2str(HRESULT hr) {
    _com_error err(hr);
    return std::string(CW2A(err.ErrorMessage()));
}

std::string Utility::boolArr2str(const std::unique_ptr<bool[]>& boolArray, size_t size) {
    std::string str(size, '0');
    for (size_t i = 0;
i < size;
++i) {
        if (boolArray[i]) {
            str[size - 1 - i] = '1';
        }
    }
    return str;
}

std::string Utility::floatArr2str(const std::unique_ptr<float[]>& floatArray, size_t size, char separator) {
    std::string str;
    str.reserve(size * 8); // Rough estimate of space required (considering float representation)

    for (size_t i = 0; i < size; ++i) {
        if (i != 0) {
            str += separator;
        }
        str += std::to_string(floatArray[i]);
    }
    return str;
}
