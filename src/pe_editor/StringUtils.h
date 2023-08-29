#pragma once

#include <string>

namespace pe_editor::StringUtils {

using UINT = unsigned int;
#define CP_UTF8 65001

std::wstring str2wstr(std::string_view str, UINT codePage = CP_UTF8);

std::string wstr2str(std::wstring_view wstr);

} // namespace pe_editor::StringUtils
