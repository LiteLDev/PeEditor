#include "pe_editor/ChooseFileUtil.h"
#include "pe_editor/StringUtils.h"

#include <ShlObj_core.h>
#include <iostream>
#include <windows.h>

namespace pe_editor::ChooseFileUtil {

std::optional<std::filesystem::path> chooseFile(const wchar_t* title, const wchar_t* filter) {
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    wchar_t fileName[MAX_PATH] = {0};
    ofn.lStructSize            = sizeof(ofn);
    ofn.lpstrFilter            = filter;
    ofn.lpstrFile              = fileName;
    ofn.nMaxFile               = MAX_PATH;
    ofn.lpstrTitle             = title;
    ofn.Flags                  = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        std::filesystem::path path = fileName;
        return path;
    }
    return std::nullopt;
}
} // namespace pe_editor::ChooseFileUtil
