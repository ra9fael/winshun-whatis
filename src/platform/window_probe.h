#pragma once
#include "src/core/selection.h"
#include <windows.h>

namespace whatis {
std::string utf8(const std::wstring& value);
std::wstring wide(const std::string& value);
Identity identify(HWND hwnd);
HWND hitWindow(POINT point, DWORD excludedPid);
RECT frameRect(HWND hwnd);
Result inspect(Identity identity);
bool stillSame(const Result& result);
bool inputDesktopAvailable();
void enableDpiAwareness();
}
