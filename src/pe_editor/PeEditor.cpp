#include "pe_editor/PeEditor.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string_view>

#include "cxxopts.hpp"
#include "demangler/Demangle.h"
#include "snappy.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "pe_editor/ChooseFileUtil.h"
#include "pe_editor/CxxOptAdder.h"
#include "pe_editor/FakeSymbol.h"
#include "pe_editor/Filter.h"
#include "pe_editor/StringUtils.h"

#include <windows.h>

namespace pe_editor {

inline void pause() {
    if (config::shouldPause) system("pause");
}

inline void exitWith(int code) {
    pause();
    std::exit(code);
}

inline std::string getPathUtf8(const std::filesystem::path& path) { return StringUtils::wstr2str(path.wstring()); }

void parseArgs(int argc, char** argv) {
    using cxxopts::value;

    cxxopts::Options options("PeEditor", "LeviLamina ToolChain PeEditor " PE_EDITOR_VERSION);
    options.allow_unrecognised_options();
    options.set_width(-1);

    CxxOptsAdder(options)
        .add(
            "m,mod",
            "Generate bedrock_server_mod.exe (will be true if no arg passed)",
            value<bool>()->default_value("false")
        )
        .add("p,pause", "Pause before exit (will be true if no arg passed)", value<bool>()->default_value("false"))
        .add("O,old", "Use old mode for LeviLamina", value<bool>()->default_value("false"))
        .add(
            "b,bak",
            "Add a suffix \".bak\" to original server exe (will be true if no arg passed)",
            value<bool>()->default_value("false")
        )
        .add("d,def", "Generate def files for develop propose", value<bool>()->default_value("false"))
        .add("l,lib", "Generate lib files for develop propose", value<bool>()->default_value("false"))
        .add("r,rebuild-pdb", "Generate pdb file for debug propose", value<bool>()->default_value("false"))
        .add("s,sym", "Generate symbol list containing symbol and rva", value<bool>()->default_value("false"))
        .add("t,data", "Generate symbol data containing symbol and rva", value<bool>()->default_value("false"))
        .add("o,output-dir", "Output dir", value<std::string>()->default_value("./"))
        .add("exe", "BDS executable file name", value<std::string>()->default_value("./bedrock_server.exe"))
        .add("pdb", "BDS debug database file name", value<std::string>()->default_value("./bedrock_server.pdb"))
        .add("idata", "Symbol database file name", value<std::string>()->default_value("./bedrock_symbol_data"))
        .add("c,choose-pdb-file", "Choose PDB file with a window", value<bool>()->default_value("false"))
        .add("verbose", "Verbose output", value<bool>()->default_value("false"))
        .add("V,version", "Print version", value<bool>()->default_value("false"))
        .add("h,help", "Print usage");

    auto optionsResult = options.parse(argc, argv);

    if (!optionsResult.unmatched().empty()) {
        std::cout << "Unknown options: " << std::endl;
        for (const auto& t : optionsResult.unmatched()) {
            std::cout << "\t" << t << std::endl;
        }
        exit(-1);
    }

    if (optionsResult.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    if (optionsResult.count("version")) {
        logger->info("LeviLamina ToolChain PeEditor");
        logger->info("Build Date CST " __TIMESTAMP__);
        exit(0);
    }

    if (optionsResult.count("verbose")) {
        logger->set_level(spdlog::level::debug);
    }

    config::genModdedBds  = optionsResult["mod"].as<bool>();
    config::genDefFile    = optionsResult["def"].as<bool>();
    config::genLibFile    = optionsResult["lib"].as<bool>();
    config::genPdbFile    = optionsResult["rebuild-pdb"].as<bool>();
    config::genSymbolList = optionsResult["sym"].as<bool>();
    config::genSymbolData = optionsResult["data"].as<bool>();
    config::backupBds     = optionsResult["bak"].as<bool>();
    config::shouldPause   = optionsResult["pause"].as<bool>();
    config::choosePdbFile = optionsResult["choose-pdb-file"].as<bool>();
    config::outputDir     = StringUtils::str2u8strConst(optionsResult["output-dir"].as<std::string>());
    config::bdsExePath    = StringUtils::str2u8strConst(optionsResult["exe"].as<std::string>());
    config::bdsPdbPath    = StringUtils::str2u8strConst(optionsResult["pdb"].as<std::string>());
    config::inputdataPath = StringUtils::str2u8strConst(optionsResult["idata"].as<std::string>());
}

bool filterSymbols(const PdbSymbol& symbol) {
    if (!symbol.isPublic || symbol.verbose) {
        return false;
    }
    if (symbol.name[0] != '?') {
        return false;
    }
    for (const auto& a : pe_editor::filter::prefix) {
        if (symbol.name.starts_with(a)) return false;
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
        logger->error("Cannot create bedrock_server_api.def.");
        return -1;
    }
    defFiles.varFile.open(config::outputDir / config::defVarFile, std::ios::ate | std::ios::out);
    if (!defFiles.varFile) {
        logger->error("Cannot create bedrock_server_var.def.");
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
        record.Name = pe_editor::FakeSymbol::getFakeSymbol(symbol.name).value_or(symbol.name);
        if (!symbol.isFunction) {
            VarExports.push_back(record);
        } else {
            ApiExports.push_back(record);
            if (auto removeVirtual = pe_editor::FakeSymbol::getFakeSymbol(symbol.name, true); removeVirtual) {
                record.Name = *removeVirtual;
                ApiExports.push_back(record);
            }
        }
    });

    auto err = llvm::object::writeImportLibrary(
        "bedrock_server.dll",
        (config::outputDir / config::libApiFile).string(),
        ApiExports,
        static_cast<llvm::COFF::MachineTypes>(IMAGE_FILE_MACHINE_AMD64),
        true
    );
    if (err) {
        logger->error("Cannot create bedrock_server_api.lib.");
        return -1;
    }
    logger->info("Generated bedrock_server_api.lib successfully. Exported {} symbols.", ApiExports.size());
    err = llvm::object::writeImportLibrary(
        "bedrock_server_mod.exe",
        (config::outputDir / config::libVarFile).string(),
        VarExports,
        static_cast<llvm::COFF::MachineTypes>(IMAGE_FILE_MACHINE_AMD64),
        true
    );
    if (err) {
        logger->error("Cannot create bedrock_server_var.lib.");
        return -1;
    }
    logger->info("Generated bedrock_server_var.lib successfully. Exported {} symbols.", VarExports.size());
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
        logger->error("Cannot create symbol list file.");
        return -1;
    }
    std::ranges::for_each(*symbols, [&](const PdbSymbol& fn) {
        if (fn.verbose) {
            return;
        }
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

void writeVarint(uint32_t v, std::string& data) {
    if ((v & (0xFFFFFFFF << 7)) == 0) {
        data += uint8_t(v);
    } else {
        data += uint8_t(v & 0x7F | 0x80);
        if ((v & (0xFFFFFFFF << 14)) == 0) {
            data += uint8_t(v >> 7);
        } else {
            data += uint8_t((v >> 7) & 0x7F | 0x80);
            if ((v & (0xFFFFFFFF << 21)) == 0) {
                data += uint8_t(v >> 14);
            } else {
                data += uint8_t((v >> 14) & 0x7F | 0x80);
                if ((v & (0xFFFFFFFF << 28)) == 0) {
                    data += uint8_t(v >> 21);
                } else {
                    data += uint8_t((v >> 21) & 0x7F | 0x80);
                    data += uint8_t(v >> 28);
                }
            }
        }
    }
}

int generateSymbolDataFile() {
    using namespace data;
    if (!config::genSymbolData) {
        return 0;
    }
    logger->info("Generating symbol data file...");
    symbolDataFile.open(config::outputDir / config::symbolDataFile, std::ios::ate | std::ios::out | std::ios::binary);
    if (!symbolDataFile) {
        logger->error("Cannot create symbol data file.");
        return -1;
    }
    std::multimap<uint32_t, PdbSymbol const*> sortedSymbols;
    std::ranges::for_each(*symbols, [&](const PdbSymbol& fn) {
        if (fn.name.empty()) {
            return;
        }
        sortedSymbols.insert({fn.rva, &fn});
    });
    std::string uncompressed, compressed;

    uint32_t lastRva{0};
    std::ranges::for_each(sortedSymbols | std::views::values, [&](const PdbSymbol* fn) {
        uint8_t starts{};
        starts        |= (uint8_t(fn->isFunction) * (1 << 0));
        starts        |= (uint8_t(fn->isPublic) * (1 << 1));
        starts        |= (uint8_t(fn->verbose) * (1 << 2));
        uncompressed  += starts;
        uint32_t rrva  = fn->rva - lastRva;
        lastRva        = fn->rva;
        writeVarint(rrva, uncompressed);
        uncompressed += fn->name;
        uncompressed += '\n';
    });
    snappy::Compress(uncompressed.data(), uncompressed.size(), &compressed);
    symbolDataFile << compressed;
    symbolDataFile.flush();
    symbolDataFile.close();
    logger->info("Generated symbol data file successfully.");
    return 0;
}

struct ImportDllName {
    static constexpr auto levilamina = "PreLoader.dll";
};

struct ImportFunctionName {
    static constexpr auto levilamina = "pl_resolve_symbol";
};

int generateModdedBds() {
    using namespace data;
    using namespace pe_bliss;
    if (!config::genModdedBds) {
        return 0;
    }

    logger->info("Loading BDS executable file...");
    originBds.file.open(config::bdsExePath, std::ios::in | std::ios::binary);
    if (!data::originBds.file) {
        logger->error("Failed to open BDS executable file.");
        return -1;
    }
    try {
        originBds.pe                = std::make_unique<pe_base>(pe_factory::create_pe(originBds.file));
        originBds.exportedFunctions = get_exported_functions(*originBds.pe, originBds.exportInfo);
        originBds.ordinalBase       = get_export_ordinal_limits(originBds.exportedFunctions).second;
    } catch (const pe_exception& e) {
        logger->error("Failed to parse BDS executable file: {}", e.what());
        return -1;
    }
    logger->info("Loaded BDS executable file successfully.");
    logger->info("Generating modified BDS executable file...");

    auto                            exportOrdinal = data::originBds.ordinalBase + 1;
    std::unordered_set<std::string> exportedFunctionsNames;

    auto names = originBds.exportedFunctions
               | std::views::filter([](const exported_function& fn) { return fn.has_name(); })
               | std::views::transform([](const exported_function& fn) { return fn.get_name(); });
    std::ranges::copy(names, std::inserter(exportedFunctionsNames, exportedFunctionsNames.end()));

    auto filtered =
        data::filteredSymbols | std::views::filter([&](const PdbSymbol& symbol) { return !symbol.isFunction; });

    for (const auto& symbol : filtered) {
        auto fakeSymbol = pe_editor::FakeSymbol::getFakeSymbol(symbol.name).value_or(symbol.name);
        if (exportedFunctionsNames.contains(fakeSymbol)) {
            continue;
        }
        exportedFunctionsNames.insert(fakeSymbol);
        exported_function func;

        func.set_name(fakeSymbol);
        func.set_rva(symbol.rva);
        func.set_ordinal(exportOrdinal++);
        if (exportOrdinal > 65535) {
            logger->error("Too many symbols to insert to ExportTable.");
            return -1;
        }
        originBds.exportedFunctions.push_back(func);
    }
    std::ofstream moddedBds(config::outputDir / "bedrock_server_mod.exe", std::ios::binary);
    if (!moddedBds) {
        logger->error("Cannot create modifed BDS executable file.");
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
        func.set_name(ImportFunctionName::levilamina);
        func.set_iat_va(0x1);
        preLoader.set_name(ImportDllName::levilamina);
        preLoader.add_import(func);
        imports.push_back(preLoader);
        section& attachedImportedSection = originBds.pe->add_section(ImportSection);
        rebuild_imports(*originBds.pe, imports, attachedImportedSection, import_rebuilder_settings(true, false));

        rebuild_pe(*originBds.pe, moddedBds);

        moddedBds.close();
        originBds.file.close();
        logger->info("Generated modified BDS executable file successfully. Exported {} symbols.", exportOrdinal);

        if (!config::backupBds) {
            return 0;
        }

        std::error_code ec;
        auto            newBdsPath = config::bdsExePath;
        newBdsPath.replace_extension(L".exe.bak");
        std::filesystem::rename(config::bdsExePath, newBdsPath, ec);
        if (ec) {
            logger->error("Failed to backup BDS executable file: {}", ec.message());
            return -1;
        }
        logger->info("Backed up BDS executable file successfully.");

    } catch (pe_bliss::pe_exception& e) {
        logger->error("Failed to build modified BDS executable file: {}", e.what());
        moddedBds.close();
        originBds.file.close();
        std::filesystem::remove(config::outputDir / "bedrock_server_mod.exe");
    }
    return 0;
}

std::unique_ptr<std::deque<PdbSymbol>> loadData(std::filesystem::path const& path) {
    std::ifstream fRead;
    fRead.open(path, std::ios_base::in | std::ios_base::binary);
    if (!fRead.is_open()) {
        return nullptr;
    }
    auto        func = std::make_unique<std::deque<PdbSymbol>>();
    std::string compressed((std::istreambuf_iterator<char>(fRead)), {});
    std::string data;
    snappy::Uncompress(compressed.data(), compressed.size(), &data);
    uint32_t rva{0};
    for (size_t i = 0; i < data.size(); i++) {
        uint8_t    c          = data[i++];
        bool const isFunction = c & (1 << 0);
        bool const isPublic   = c & (1 << 1);
        bool const verbose    = c & (1 << 2);
        uint32_t   rrva{0};
        int        shift_amount = 0;
        do {
            rrva         |= (uint32_t)(data[i] & 0x7F) << shift_amount;
            shift_amount += 7;
        } while ((data[i++] & 0x80) != 0);
        rva        += rrva;
        auto begin  = i;
        i           = data.find_first_of('\n', begin);
        if (i == std::string::npos) {
            break;
        }
        std::string name(data, begin, i - begin);
        func->push_back(PdbSymbol(std::move(name), rva, isFunction, isPublic, verbose));
    }
    fRead.close();
    return func;
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
        config::backupBds    = true;
        config::shouldPause  = true;
    }

    logger->info("LeviLamina ToolChain PeEditor " PE_EDITOR_VERSION);
    logger->info("Build Date CST " __TIMESTAMP__);

    // exit if no work to do
    if (!config::genModdedBds && !config::genLibFile && !config::genDefFile && !config::genSymbolList
        && !config::genSymbolData && !config::genPdbFile) {
        logger->info("No work to do, exiting...");
        return 0;
    }

    if (config::choosePdbFile) {
        logger->info("Choosing PDB file...");
        auto title  = L"Choose a PDB file of bedrock_server.exe";
        auto filter = L"PDB File (*.pdb)\0*.pdb\0Any File (*.*)\0*.*\0";
        auto ret    = ChooseFileUtil::chooseFile(title, filter);
        if (ret.has_value()) {
            config::bdsPdbPath = ret.value();
        }
    }

    logger->info("Configurations:");
    logger->info("\tBDS Executable File: \t\t[{}]", getPathUtf8(config::bdsExePath.wstring()));
    logger->info("\tBDS PDB File: \t\t\t[{}]", getPathUtf8(config::bdsPdbPath.wstring()));
    logger->info("\tOutput Dir: \t\t\t[{}]", getPathUtf8(config::outputDir.wstring()));
    logger->info("Targets:");
    logger->info("\tModified BDS Executable: \t[{}]", config::genModdedBds);
    logger->info("\tModule Definition Files: \t[{}]", config::genDefFile);
    logger->info("\tImport Library Files: \t\t[{}]", config::genLibFile);
    logger->info("\tSymbol List File: \t\t[{}]", config::genSymbolList);

    logger->info("Loading PDB file...");
    data::symbols = loadPDB(config::bdsPdbPath.wstring().c_str());
    if (!data::symbols && !std::filesystem::exists(config::inputdataPath)) {
        logger->error("Failed to load PDB file.");
        exitWith(-1);
    } else if (!data::symbols) {
        logger->info("Loading Data file...");
        if (data::symbols = pe_editor::loadData(config::inputdataPath); !data::symbols) {
            logger->error("Failed to load Data file.");
            exitWith(-1);
        }
    }
    logger->info("Loaded {} symbols.", data::symbols->size());

    if (auto ret = generateSymbolListFile()) {
        exitWith(ret);
    }
    if (auto ret = generateSymbolDataFile()) {
        exitWith(ret);
    }

    if (config::genModdedBds || config::genDefFile || config::genLibFile) {
        logger->info("Filtering symbols...");
        std::ranges::copy_if(*data::symbols, std::back_inserter(data::filteredSymbols), filterSymbols);
        logger->info("Filtered {} symbols.", data::filteredSymbols.size());
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
