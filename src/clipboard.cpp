#include <clipboard.hpp>
#include <windows.h>

bool Clipboard::copyText(const std::string& text, std::string& error)
{
    error.clear();

    if (!OpenClipboard(nullptr)) {
        error = "Failed to open clipboard";
        return false;
    }

    if (!EmptyClipboard()) {
        CloseClipboard();
        error = "Failed to clear clipboard";
        return false;
    }

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wideLen <= 0) {
        CloseClipboard();
        error = "Failed to convert text";
        return false;
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(wideLen) * sizeof(wchar_t));
    if (!hMem) {
        CloseClipboard();
        error = "Failed to allocate clipboard memory";
        return false;
    }

    wchar_t* buffer = static_cast<wchar_t*>(GlobalLock(hMem));
    if (!buffer) {
        GlobalFree(hMem);
        CloseClipboard();
        error = "Failed to lock clipboard memory";
        return false;
    }

    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, buffer, wideLen);
    GlobalUnlock(hMem);

    if (!SetClipboardData(CF_UNICODETEXT, hMem)) {
        GlobalFree(hMem);
        CloseClipboard();
        error = "Failed to set clipboard data";
        return false;
    }

    CloseClipboard();
    return true;
}
