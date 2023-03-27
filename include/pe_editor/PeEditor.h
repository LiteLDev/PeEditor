#pragma once

#include <filesystem>
#include <fstream>
#include <memory>

// fuck windows.h
#include "llvm/Object/COFF.h"
#include "llvm/Object/COFFImportFile.h"

#include "pe_bliss/pe_bliss.h"
#include "spdlog/spdlog.h"

#include "pe_editor/Pdb.h"

#define PE_EDITOR_VERSION "v3.1.1"

namespace pe_editor {

inline std::shared_ptr<spdlog::logger> logger;

namespace config {
inline bool genModdedBds  = false;
inline bool genDefFile    = false;
inline bool genLibFile    = false;
inline bool genSymbolList = false;
inline bool backupBds     = false;
inline bool shouldPause   = false;
inline bool choosePdbFile = false;
inline bool liteloader3   = false;

inline std::filesystem::path outputDir;
inline std::filesystem::path bdsExePath;
inline std::filesystem::path bdsPdbPath;

constexpr auto defApiFile     = "bedrock_server_api.def";
constexpr auto defVarFile     = "bedrock_server_var.def";
constexpr auto libApiFile     = "bedrock_server_api.lib";
constexpr auto libVarFile     = "bedrock_server_var.lib";
constexpr auto symbolListFile = "bedrock_server_symbol_list.txt";

} // namespace config

namespace data {

inline std::unique_ptr<std::deque<PdbSymbol>> symbols;
inline std::deque<PdbSymbol>                  filteredSymbols;

inline struct {
    std::ifstream                      file;
    std::unique_ptr<pe_bliss::pe_base> pe;
    pe_bliss::export_info              exportInfo;
    pe_bliss::exported_functions_list  exportedFunctions;
    uint16_t                           ordinalBase{};
} originBds;

inline struct {
    std::ofstream apiFile;
    std::ofstream varFile;
} defFiles;

inline std::ofstream symbolListFile;

} // namespace data

void parseArgs(int argc, char** argv);

bool filterSymbols(const PdbSymbol& symbol);

int generateDefFile();

int generateLibFile();

int generateSymbolListFile();

int generateModdedBds();

} // namespace pe_editor
