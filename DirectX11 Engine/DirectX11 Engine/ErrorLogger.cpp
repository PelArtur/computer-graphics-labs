#include "ErrorLogger.hpp"
#include <comdef.h>


void ErrorLogger::Log(std::string message)
{
	std::string error_message = "Error: " + message;
	// MessageBoxA -- messages are short strings(8-bit)
	// Parent window, error message, title, msg box type
	MessageBoxA(NULL, error_message.c_str(), "Error", MB_ICONERROR);
}


void ErrorLogger::Log(HRESULT hr, std::string message)
{
	_com_error error(hr);  //error object for retrieving error meassage
	std::wstring error_message = L"Error: " + StringConverter::StringToWide(message) + L"\n" + error.ErrorMessage();
	// MessageBoxW -- messages are wide strings(16-bit)
	MessageBoxW(NULL, error_message.c_str(), L"Error", MB_ICONERROR);
}


void ErrorLogger::Log(HRESULT hr, std::wstring message)
{
	_com_error error(hr);  //error object for retrieving error meassage
	std::wstring error_message = L"Error: " + message + L"\n" + error.ErrorMessage();
	// MessageBoxW -- messages are wide strings(16-bit)
	MessageBoxW(NULL, error_message.c_str(), L"Error", MB_ICONERROR);
}
