#pragma once
#include <string>

class StringHelper 
{
public:
	// Windows and Directx11 expect wide strings, because they use UTF-16 format(16-bit chars)  dsafdsaf
	static std::wstring StringToWide(std::string str);
	static std::string GetDirectoryFromPath(const std::string& filepath);
	static std::string GetFileExtension(const std::string& filename);
};