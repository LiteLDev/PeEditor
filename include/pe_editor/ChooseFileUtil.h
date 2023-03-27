#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace pe_editor::ChooseFileUtil {
std::optional<std::filesystem::path> chooseFile(const wchar_t* title, const wchar_t* filter);
} // namespace pe_editor::ChooseFileUtil
