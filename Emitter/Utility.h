#pragma once
#include <windows.h>
#include <string>

class Utility {
public:
	static std::string wstr2str(const std::wstring& wstr);
	static std::string hr2str(HRESULT hr);
	static std::string boolArr2str(const std::unique_ptr<bool[]>& boolArray, size_t size);
	static std::string floatArr2str(const std::unique_ptr<float[]>& floatArray, size_t size, char separator);
};
