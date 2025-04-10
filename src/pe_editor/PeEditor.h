#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "spdlog/logger.h"

namespace pe_editor {

inline std::shared_ptr<spdlog::logger> logger;

namespace config {
inline bool genModdedBds  = false;
inline bool backupBds     = false;
inline bool shouldPause   = false;
inline bool inplaceExe    = false;

inline std::filesystem::path outputDir;
inline std::filesystem::path bdsExePath;

} // namespace config

void parseArgs(int argc, char** argv);

int generateModdedBds();

} // namespace pe_editor
