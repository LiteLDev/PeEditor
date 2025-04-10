#include "pe_editor/PeEditor.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "cxxopts.hpp"
#include "pe_bliss/pe_base.h"
#include "pe_bliss/pe_checksum.h"
#include "pe_bliss/pe_factory.h"
#include "pe_bliss/pe_imports.h"
#include "pe_bliss/pe_rebuilder.h"
#include "pe_bliss/pe_section.h"
#include "spdlog/common.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "pe_editor/CxxOptAdder.h"
#include "pe_editor/StringUtils.h"

#include <windows.h>

#include <consoleapi2.h>
#include <winnls.h>

namespace fs = std::filesystem;

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
            "Generate mod.exe (will be true if no arg passed)",
            value<bool>()->implicit_value("true")->default_value("false")
        )
        .add(
            "p,pause",
            "Pause before exit (will be true if no arg passed)",
            value<bool>()->implicit_value("true")->default_value("false")
        )
        .add(
            "b,bak",
            "Add a suffix \".bak\" to original server exe (will be true if no arg passed)",
            value<bool>()->implicit_value("true")->default_value("false")
        )
        .add("inplace", "name inplace", value<bool>()->default_value("false"))
        .add("o,output-dir", "Output dir", value<std::string>()->default_value("./"))
        .add("exe", "executable file name", value<std::string>()->default_value("./bedrock_server.exe"))
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
        logger->info("LeviLamina ToolChain PeEditor {0}", PE_EDITOR_VERSION);
        logger->info("Build Date CST " __TIMESTAMP__);
        exit(0);
    }

    if (optionsResult.count("verbose")) {
        logger->set_level(spdlog::level::debug);
    }

    config::genModdedBds = optionsResult["mod"].as<bool>();
    config::backupBds    = optionsResult["bak"].as<bool>();
    config::shouldPause  = optionsResult["pause"].as<bool>();
    config::inplaceExe   = optionsResult["inplace"].as<bool>();
    config::outputDir    = StringUtils::str2u8strConst(optionsResult["output-dir"].as<std::string>());
    config::bdsExePath   = StringUtils::str2u8strConst(optionsResult["exe"].as<std::string>());
}

struct ImportDllName {
    static constexpr auto preloader = "PreLoader.dll";
};

struct ImportFunctionName {
    static constexpr auto preloader = "pl_resolve_symbol";
};

int generateModdedBds() {
    using namespace pe_bliss;

    if (!config::genModdedBds) {
        return 0;
    }

    logger->info("Loading executable file...");

    fs::path                 moddedPath = config::outputDir / (config::bdsExePath.stem().u8string() + u8"_mod.exe");
    std::ifstream            originFile(config::bdsExePath, std::ios::binary);
    std::unique_ptr<pe_base> pe;

    if (!originFile) {
        logger->error("Failed to open executable file.");
        return -1;
    }

    try {
        pe = std::make_unique<pe_base>(pe_factory::create_pe(originFile));
    } catch (std::exception const& e) {
        logger->error("Failed to parse executable file: {}", e.what());
        return -1;
    }

    logger->info("Loaded executable file successfully.");
    logger->info("Generating modified executable file...");

    try {
        imported_function func;
        func.set_name(ImportFunctionName::preloader);
        func.set_iat_va(0x1); // magic number

        import_library preLoader;
        preLoader.set_name(ImportDllName::preloader);
        preLoader.add_import(func);

        imported_functions_list imports = get_imported_functions(*pe);
        imports.push_back(preLoader);

        section newImportSection;
        newImportSection.get_raw_data().resize(1);
        newImportSection.set_name(".nimp");
        newImportSection.readable(true).writeable(true);

        section& moddedSection = pe->add_section(newImportSection);

        import_rebuilder_settings settings;
        settings.build_original_iat(true);
        settings.save_iat_and_original_iat_rvas(true);

        rebuild_imports(*pe, imports, moddedSection, settings);


        // Calculate new checksum
        {
            std::ofstream moddedFileOutput(moddedPath, std::ios::binary | std::ios::trunc | std::ios::out);
            rebuild_pe(*pe, moddedFileOutput);
        }
        {
            std::ifstream moddedFileInput(moddedPath, std::ios::binary | std::ios::in);
            auto          res = calculate_checksum(moddedFileInput);
            pe->set_checksum(res);
        }

        std::ofstream moddedFile(moddedPath, std::ios::binary);
        rebuild_pe(*pe, moddedFile);

        moddedFile.close();
        originFile.close();
        logger->info("Generated modified executable file successfully.");

        if (!config::backupBds) {
            return 0;
        }

        try {
            fs::path backupFilePath = config::bdsExePath;
            backupFilePath.replace_extension(L".exe.bak");
            std::filesystem::rename(config::bdsExePath, backupFilePath);
            if (config::inplaceExe) {
                std::filesystem::rename(moddedPath, config::bdsExePath);
            }
            logger->info("Backed up executable file successfully.");
        } catch (std::exception const& e) {
            logger->error("Failed to backup executable file: {}", e.what());
            return -1;
        }

    } catch (std::exception const& e) {
        logger->error("Failed to build modified executable file: {}", e.what());
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

    logger->info("LeviLamina ToolChain PeEditor " PE_EDITOR_VERSION);

    // exit if no work to do
    if (!config::genModdedBds) {
        logger->info("No work to do, exiting...");
        return 0;
    }

    logger->info("Configurations:");
    logger->info("\tBDS Executable File: \t\t[{}]", getPathUtf8(config::bdsExePath.wstring()));
    logger->info("\tOutput Dir: \t\t\t[{}]", getPathUtf8(config::outputDir.wstring()));
    logger->info("Targets:");
    logger->info("\tModified Executable: \t\t[{}]", config::genModdedBds);


    if (auto ret = generateModdedBds()) {
        exitWith(ret);
    }
    return 0;
}