#pragma once

#include <string>

namespace pe_editor::StringUtils {

using uint          = unsigned int;
constexpr uint UTF8 = 65001;

std::wstring str2wstr(std::string_view str, uint codePage = UTF8);

std::string wstr2str(std::wstring_view wstr);

} // namespace pe_editor::StringUtils
