#include "pe_editor/StringUtils.h"

#include <Windows.h>

namespace pe_editor::StringUtils {
std::wstring str2wstr(std::string_view str, uint codePage) {
    int          size = MultiByteToWideChar(codePage, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(codePage, 0, str.data(), static_cast<int>(str.size()), wstr.data(), size);
    return wstr;
}

std::string wstr2str(std::wstring_view wstr) {
    int size =
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), size, nullptr, nullptr);
    return str;
}
} // namespace pe_editor::StringUtils
