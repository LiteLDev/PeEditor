#include "pe_editor/PeEditor.h"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <string_view>

#include "cxxopts.hpp"
#include "demangler/Demangle.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "pe_editor/ChooseFileUtil.h"
#include "pe_editor/FakeSymbol.h"
#include "pe_editor/Filter.h"
#include "pe_editor/StringUtils.h"

#include <windows.h>

namespace pe_editor {

inline void pause() {
    if (config::shouldPause)
        system("pause");
}

inline void exitWith(int code) {
    pause();
    std::exit(code);
}

inline std::string getPathUtf8(const std::filesystem::path& path) { return StringUtils::wstr2str(path.wstring()); }

void parseArgs(int argc, char** argv) {
    cxxopts::Options options("PeEditor", "LiteLoaderBDS ToolChain");
    options.allow_unrecognised_options();
    using cxxopts::value;
    using std::string;
    // clang-format off
    options.add_options()
        ("m,mod", "Generate bedrock_server_mod.exe (will be true if no arg passed)", value<bool>()->default_value("false"))
        ("p,pause", "Pause before exit (will be true if no arg passed)", value<bool>()->default_value("false"))
        ("n,new", "Use LiteLoader v3 preview mode", value<bool>()->default_value("false"))
        ("d,def", "Generate def files for develop propose", value<bool>()->default_value("false"))
        ("l,lib", "Generate lib files for develop propose", value<bool>()->default_value("false"))
        ("s,sym", "Generate symbol list containing symbol and rva", value<bool>()->default_value("false"))
        ("o,output-dir", "Output dir", value<string>()->default_value("./"))
        ("exe", "BDS executable file name", value<string>()->default_value("./bedrock_server.exe"))
        ("pdb", "BDS debug database file name", value<string>()->default_value("./bedrock_server.pdb"))
        ("c,choose-pdb-file", "Choose PDB file with a window", value<bool>()->default_value("false"))
        ("v,verbose", "Verbose output", value<bool>()->default_value("false"))
        ("V,version", "Print version", value<bool>()->default_value("false"))
        ("h,help", "Print usage");
    // clang-format on
    auto optionsResult = options.parse(argc, argv);

    if (optionsResult.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    if (optionsResult.count("version")) {
        logger->info("LiteLoaderBDS ToolChain PeEditor");
        logger->info("BuildDate CST " __TIMESTAMP__);
        exit(0);
    }

    if (optionsResult.count("verbose")) {
        logger->set_level(spdlog::level::debug);
    }

    config::genModdedBds  = optionsResult["mod"].as<bool>();
    config::genDefFile    = optionsResult["def"].as<bool>();
    config::genLibFile    = optionsResult["lib"].as<bool>();
    config::genSymbolList = optionsResult["sym"].as<bool>();
    config::shouldPause   = optionsResult["pause"].as<bool>();
    config::choosePdbFile = optionsResult["choose-pdb-file"].as<bool>();
    config::liteloader3   = optionsResult["new"].as<bool>();
    config::outputDir     = optionsResult["output-dir"].as<std::string>();
    config::bdsExePath    = optionsResult["exe"].as<std::string>();
    config::bdsPdbPath    = optionsResult["pdb"].as<std::string>();
}

bool filterSymbols(const PdbSymbol& symbol) {
    if (symbol.name[0] != '?') {
        return false;
    }
    for (const auto& a : pe_editor::filter::prefix) {
        if (symbol.name.starts_with(a))
            return false;
    }
    return !pe_editor::filter::matchSkip(symbol.name);
}

int generateDefFile() {
    using namespace data;
    if (!config::genDefFile) {
        return 0;
    }
    logger->info("Generating def files...");
    defFiles.apiFile.open(config::outputDir / config::defApiFile, std::ios::ate | std::ios::out);
    if (!defFiles.apiFile) {
        logger->error("Cannot create bedrock_server_api.def");
        return -1;
    }
    defFiles.varFile.open(config::outputDir / config::defVarFile, std::ios::ate | std::ios::out);
    if (!defFiles.varFile) {
        logger->error("Cannot create bedrock_server_var.def");
        return -1;
    }
    defFiles.apiFile << "LIBRARY bedrock_server.dll\nEXPORTS\n";
    defFiles.varFile << "LIBRARY bedrock_server_mod.exe\nEXPORTS\n";
    std::ranges::for_each(filteredSymbols, [&](const PdbSymbol& symbol) {
        if (symbol.isFunction) {
            data::defFiles.apiFile << "\t" << symbol.name << "\n";
        } else {
            data::defFiles.varFile << "\t" << symbol.name << "\n";
        }
    });
    defFiles.apiFile.flush();
    defFiles.apiFile.close();
    defFiles.varFile.flush();
    defFiles.varFile.close();
    logger->info("Generated def files successfully.");
    return 0;
}

int generateLibFile() {
    using namespace data;
    if (!config::genLibFile) {
        return 0;
    }
    logger->info("Generating lib files...");
    std::vector<llvm::object::COFFShortExport> ApiExports;
    std::vector<llvm::object::COFFShortExport> VarExports;
    std::ranges::for_each(filteredSymbols, [&](const PdbSymbol& symbol) {
        llvm::object::COFFShortExport record;
        record.Name = symbol.name;
        if (!symbol.isFunction) {
            VarExports.push_back(record);
            return;
        }
        ApiExports.push_back(record);
        auto fakeSymbol = pe_editor::FakeSymbol::getFakeSymbol(symbol.name);
        if (!fakeSymbol.has_value()) {
            return;
        }
        llvm::object::COFFShortExport fakeRecord;
        fakeRecord.Name = fakeSymbol.value();
        ApiExports.push_back(fakeRecord);
    });

    auto ret = llvm::object::writeImportLibrary(
        "bedrock_server.dll",
        (config::outputDir / config::libApiFile).string(),
        ApiExports,
        static_cast<llvm::COFF::MachineTypes>(IMAGE_FILE_MACHINE_AMD64),
        true
    );
    if (ret) {
        logger->error("Cannot create bedrock_server_api.lib");
        return -1;
    }
    logger->info("Generated bedrock_server_api.lib successfully.");
    ret = llvm::object::writeImportLibrary(
        "bedrock_server_mod.exe",
        (config::outputDir / config::libVarFile).string(),
        VarExports,
        static_cast<llvm::COFF::MachineTypes>(IMAGE_FILE_MACHINE_AMD64),
        true
    );
    if (ret) {
        logger->error("Cannot create bedrock_server_var.lib");
        return -1;
    }
    logger->info("Generated bedrock_server_var.lib successfully.");
    return 0;
}

int generateSymbolListFile() {
    using namespace data;
    if (!config::genSymbolList) {
        return 0;
    }
    logger->info("Generating symbol list file...");
    symbolListFile.open(config::outputDir / config::symbolListFile, std::ios::ate | std::ios::out);
    if (!symbolListFile) {
        logger->error("Cannot create Symbol List File");
        return -1;
    }
    std::ranges::for_each(*symbols, [&](const PdbSymbol& fn) {
        auto demangledName = demangler::microsoftDemangle(
            fn.name.c_str(),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            (demangler::MSDemangleFlags)(demangler::MSDF_NoCallingConvention | demangler::MSDF_NoAccessSpecifier)
        );

        if (!demangledName) {
            demangledName = _strdup("Failed to demangle.");
        }
        symbolListFile << fmt::format("[{:08x}] {}\n{}\n\n", fn.rva, fn.name, demangledName);
        free(demangledName);
    });
    symbolListFile.flush();
    symbolListFile.close();
    logger->info("Generated symbol list file successfully.");
    return 0;
}

struct ImportDllName {
    static constexpr const char* liteloader2 = "LLPreLoader.dll";
    static constexpr const char* liteloader3 = "PreLoader.dll";
};

struct ImportFunctionName {
    static constexpr const char* liteloader2 = "dlsym_real";
    static constexpr const char* liteloader3 = "pl_resolve_symbol";
};

int generateModdedBds() {
    using namespace data;
    using namespace pe_bliss;
    if (!config::genModdedBds) {
        return 0;
    }

    logger->info("Loading bedrock_server.exe...");
    originBds.file.open(config::bdsExePath, std::ios::in | std::ios::binary);
    if (!data::originBds.file) {
        logger->error("Failed to Open bedrock_server.exe");
        return -1;
    }
    try {
        originBds.pe                = std::make_unique<pe_base>(pe_factory::create_pe(originBds.file));
        originBds.exportedFunctions = get_exported_functions(*originBds.pe, originBds.exportInfo);
        originBds.ordinalBase       = get_export_ordinal_limits(originBds.exportedFunctions).second;
    } catch (const pe_exception& e) {
        logger->error("Failed to parse bedrock_server.exe: {}", e.what());
        return -1;
    }
    logger->info("Loaded bedrock_server.exe successfully.");
    logger->info("Generating modded bedrock_server.exe...");

    auto                            exportOrdinal = data::originBds.ordinalBase + 1;
    std::unordered_set<std::string> exportedFunctionsNames;

    auto names = originBds.exportedFunctions |
                 std::views::filter([](const exported_function& fn) { return fn.has_name(); }) |
                 std::views::transform([](const exported_function& fn) { return fn.get_name(); });
    std::ranges::copy(names, std::inserter(exportedFunctionsNames, exportedFunctionsNames.end()));

    auto filtered = data::filteredSymbols | std::views::filter([&](const PdbSymbol& symbol) {
                        return !symbol.isFunction && !exportedFunctionsNames.contains(symbol.name);
                    });

    for (const auto& symbol : filtered) {
        exportedFunctionsNames.insert(symbol.name);
        exported_function func;
        func.set_name(symbol.name);
        func.set_rva(symbol.rva);
        func.set_ordinal(exportOrdinal++);
        if (exportOrdinal > 65535) {
            logger->error("Too many Symbols to insert to ExportTable");
            return -1;
        }
        originBds.exportedFunctions.push_back(func);
    }
    std::ofstream moddedBds(config::outputDir / "bedrock_server_mod.exe", std::ios::binary);
    if (!moddedBds) {
        logger->error("Cannot create modded bedrock_server.exe");
        return -1;
    }
    try {
        section ExportSection;
        ExportSection.get_raw_data().resize(1);
        ExportSection.set_name(".nexp");
        ExportSection.readable(true);
        section& attachedExportedSection = originBds.pe->add_section(ExportSection);
        rebuild_exports(*originBds.pe, originBds.exportInfo, originBds.exportedFunctions, attachedExportedSection);

        section ImportSection;
        ImportSection.get_raw_data().resize(1);
        ImportSection.set_name(".nimp");
        ImportSection.readable(true).writeable(true);
        imported_functions_list imports = get_imported_functions(*originBds.pe);
        import_library          preLoader;
        imported_function       func;
        func.set_name(config::liteloader3 ? ImportFunctionName::liteloader3 : ImportFunctionName::liteloader2);
        func.set_iat_va(0x1);
        preLoader.set_name(config::liteloader3 ? ImportDllName::liteloader3 : ImportDllName::liteloader2);
        preLoader.add_import(func);
        imports.push_back(preLoader);
        section& attachedImportedSection = originBds.pe->add_section(ImportSection);
        rebuild_imports(*originBds.pe, imports, attachedImportedSection, import_rebuilder_settings(true, false));

        rebuild_pe(*originBds.pe, moddedBds);
        moddedBds.close();
        logger->info("Generated bedrock_server_mod.exe successfully.");
    } catch (pe_bliss::pe_exception& e) {
        logger->error("Failed to rebuild bedrock_server_mod.exe: {}", e.what());
        moddedBds.close();
        std::filesystem::remove(config::outputDir / "bedrock_server_mod.exe");
    }
    return 0;
}

} // namespace pe_editor

int main(int argc, char** argv) {
    using namespace pe_editor;

    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    logger = spdlog::stdout_color_mt("console");
    logger->set_level(spdlog::level::info);

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    parseArgs(argc, argv);

    if (argc == 1) {
        config::genModdedBds = true;
        config::shouldPause  = true;
    }

    logger->info("LiteLoaderBDS ToolChain PeEditor v3.0.0 🥰");
    logger->info("BuildDate CST " __TIMESTAMP__);

    // exit if no work to do
    if (!config::genModdedBds && !config::genLibFile && !config::genDefFile && !config::genSymbolList) {
        logger->info("No work to do, exiting...");
        return 0;
    }

    if (config::choosePdbFile) {
        logger->info("Choosing PDB file...");
        auto title  = L"Choose a PDB file";
        auto filter = L"PDB File (*.pdb)\0*.pdb\0Any File (*.*)\0*.*\0";
        auto ret    = ChooseFileUtil::chooseFile(title, filter);
        if (ret.has_value()) {
            config::bdsPdbPath = ret.value();
        }
    }

    logger->info("LiteLoader v3 mode: \t\t[{}]", config::liteloader3);
    logger->info("BDS Executable File: \t\t[{}]", getPathUtf8(config::bdsExePath.wstring()));
    logger->info("BDS PDB File: \t\t\t[{}]", getPathUtf8(config::bdsPdbPath.wstring()));
    logger->info("Output Dir: \t\t\t[{}]", getPathUtf8(config::outputDir.wstring()));
    logger->info("Generate bedrock_server_mod.exe: \t[{}]", config::genModdedBds);
    logger->info("Generate def files: \t\t[{}]", config::genDefFile);
    logger->info("Generate lib files: \t\t[{}]", config::genLibFile);
    logger->info("Generate symbol list file: \t[{}]", config::genSymbolList);

    logger->info("Loading PDB file...");
    data::symbols = loadPDB(config::bdsPdbPath.wstring().c_str());
    if (!data::symbols) {
        logger->error("Failed to load PDB file.");
        exitWith(-1);
    }
    logger->info("Loaded {} Symbols.", data::symbols->size());

    if (auto ret = generateSymbolListFile()) {
        exitWith(ret);
    }

    if (config::genModdedBds || config::genDefFile || config::genLibFile) {
        logger->info("Filtering symbols...");
        std::ranges::copy_if(*data::symbols, std::back_inserter(data::filteredSymbols), filterSymbols);
        logger->info("Filtered {} Symbols.", data::filteredSymbols.size());
    }

    if (auto ret = generateDefFile()) {
        exitWith(ret);
    }

    if (auto ret = generateLibFile()) {
        exitWith(ret);
    }

    if (auto ret = generateModdedBds()) {
        exitWith(ret);
    }

    logger->info("Done.");
    exitWith(0);
}