#pragma once

#include <string>

namespace pe_editor::StringUtils {

using uint          = unsigned int;
constexpr uint UTF8 = 65001;

std::wstring str2wstr(std::string_view str, uint codePage = UTF8);

std::string wstr2str(std::wstring_view wstr);

[[nodiscard]] inline std::u8string const& str2u8strConst(std::string const& str) {
    return *reinterpret_cast<const std::u8string*>(&str);
}
} // namespace pe_editor::StringUtils
