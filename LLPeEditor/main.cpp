#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <windows.h>

#include <algorithm>
#include <fstream>
#include <unordered_set>

#include <llvm/Demangle/Demangle.h>
#include <llvm/Demangle/MicrosoftDemangle.h>

#include "skipFunctions.h"

#include "cxxopts.hpp"

#include "pdb.h"

#include "../PortableExecutable/pe_bliss.h"

#include <windows.h>

std::wstring str2wstr(const std::string& str, UINT codePage = CP_UTF8) {
	auto len = MultiByteToWideChar(codePage, 0, str.c_str(), -1, nullptr, 0);
	auto* buffer = new wchar_t[len + 1];
	MultiByteToWideChar(codePage, 0, str.c_str(), -1, buffer, len + 1);
	buffer[len] = L'\0';

	std::wstring result(buffer);
	delete[] buffer;
	return result;
}

using namespace pe_bliss;
inline void Pause(bool Pause) {
	if (Pause)
		system("pause");
}

void ProcessLibFile(std::string libName, std::string modExeName) {
	std::ifstream libFile;
	std::ofstream outLibFile;

	libFile.open(libName, std::ios::in | std::ios::binary);
	if (!libFile) {
		std::cout << "[Err] Cannot open " << libName << std::endl;
		Pause(true);
		return;
	}
	bool saveFlag = false;
	pe_base* LiteLib_PE = new pe_base(pe_factory::create_pe(libFile));
	try {
		imported_functions_list imports(get_imported_functions(*LiteLib_PE));
		for (int i = 0; i < imports.size(); i++) {
			if (imports[i].get_name() == "bedrock_server_mod.exe") {
				imports[i].set_name(modExeName);
				std::cout << "[INFO] Modding dll file " << libName << std::endl;
				saveFlag = true;
			}
		}

		if (saveFlag) {
			section ImportSection;
			ImportSection.get_raw_data().resize(1);
			ImportSection.set_name("ImpFunc");
			ImportSection.readable(true).writeable(true);
			section& attachedImportedSection = LiteLib_PE->add_section(ImportSection);
			pe_bliss::rebuild_imports(*LiteLib_PE, imports, attachedImportedSection, import_rebuilder_settings(true, false));

			outLibFile.open("LiteLoader.tmp", std::ios::out | std::ios::binary | std::ios::trunc);
			if (!outLibFile) {
				std::cout << "[Err] Cannot create LiteLoader.tmp!" << std::endl;
				Pause(true);
				return;
			}
			std::cout << "[INFO] Writting dll file " << libName << std::endl;
			rebuild_pe(*LiteLib_PE, outLibFile);
			libFile.close();
			std::filesystem::remove(std::filesystem::path(libName));
			outLibFile.close();
			std::filesystem::rename(std::filesystem::path("LiteLoader.tmp"), std::filesystem::path(libName));
		}
	}
	catch (const pe_exception& e) {
		std::cout << "[Err] Set ImportName Failed: " << e.what() << std::endl;
		Pause(true);
		return;
	}

}

void ProcessLibDirectory(std::string directoryName, std::string outputExeFilename) {
	std::filesystem::directory_iterator directory(directoryName);

	for (auto& file : directory) {
		if (!file.is_regular_file()) {
			continue;
		}
		std::filesystem::path path = file.path();
		std::u8string u8Name = path.filename().u8string();
		std::u8string u8Ext = path.extension().u8string();
		std::string name = reinterpret_cast<std::string&>(u8Name);
		std::string ext = reinterpret_cast<std::string&>(u8Ext);

		// Check is dll
		if (ext != ".dll") {
			continue;
		}

		std::string pluginName;
		pluginName.append(directoryName);
		pluginName.append("\\");
		pluginName.append(name);
		ProcessLibFile(pluginName, outputExeFilename);
	}
}

int main(int argc, char** argv) {
	if (argc == 1){
		SetConsoleCtrlHandler([](DWORD signal) -> BOOL {return TRUE; }, TRUE);
		EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE,
			MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}

	cxxopts::Options options("LLPeEditor", "BDSLiteLoader PE Editor For BDS");
	options.allow_unrecognised_options();
	options.add_options()
		("noMod", "Do not generate bedrock_server_mod.exe")
		("d,def", "Generate def files for develop propose")
		("noPause", "Do not pause before exit")
		("defApi", "Def File name for API Definitions", cxxopts::value<std::string>()
			->default_value("bedrock_server_api.def"))
		("defVar", "Def File name for Variable Definitions", cxxopts::value<std::string>()
			->default_value("bedrock_server_var.def"))
		("s,sym", "Generate symlist that contains symbol and it's rva", cxxopts::value<std::string>()
			->default_value("bedrock_server_symlist.txt")
			->implicit_value("bedrock_server_symlist.txt"))
		("exe", "BDS executeable file name", cxxopts::value<std::string>()
			->default_value("bedrock_server.exe"))
		("o,out", "LL executeable output file name", cxxopts::value<std::string>()
			->default_value("bedrock_server_mod.exe"))
		("pdb", "BDS debug database file name", cxxopts::value<std::string>()
			->default_value("bedrock_server.pdb"))
		("h,help", "Print usage");
	auto optionsResult = options.parse(argc, argv);

	if (optionsResult.count("help")){
		std::cout << options.help() << std::endl;
		exit(0);
	}

	bool mGenModBDS = !optionsResult["noMod"].as<bool>();
	bool mGenDevDef = optionsResult["def"].as<bool>();
	bool mGenSymbolList = optionsResult.count("sym");
	bool mPause = !optionsResult["noPause"].as<bool>();

	std::string mExeFile = optionsResult["exe"].as<std::string>();
	std::string mOutputExeFile = optionsResult["out"].as<std::string>();
	std::string mPdbFile = optionsResult["pdb"].as<std::string>();
	std::string mDefApiFile = optionsResult["defApi"].as<std::string>();
	std::string mDefVarFile = optionsResult["defVar"].as<std::string>();
	std::string mSymFile = optionsResult["sym"].as<std::string>();

	std::cout << "[Info] LiteLoader ToolChain PEEditor" << std::endl;
	std::cout << "[Info] BuildDate CST " __TIMESTAMP__ << std::endl;
	std::cout << "[Info] Gen mod executable                      " << std::boolalpha << std::string(mGenModBDS ? " true" : "false") << std::endl;
	std::cout << "[Info] Gen bedrock_server_mod.def        [DEV] " << std::boolalpha << std::string(mGenDevDef ? " true" : "false") << std::endl;
	std::cout << "[Info] Gen SymList file                  [DEV] " << std::boolalpha << std::string(mGenSymbolList ? " true" : "false") << std::endl;
	std::cout << "[Info] Output: " << mOutputExeFile << std::endl;
	
	std::cout << "[Info] Loading PDB" << std::endl;
	auto FunctionList = loadPDB(str2wstr(mPdbFile).c_str());
	std::cout << "[Info] Loaded " << FunctionList->size() << " Symbols" << std::endl;
	std::cout << "[Info] Generating BDS Please wait for few minutes" << std::endl;
	
	std::ifstream             OriginalBDS;
	std::ifstream			  LiteLib;
	std::ofstream             ModifiedBDS;
	std::ofstream             ModdedLiteLib;
	pe_base* OriginalBDS_PE = nullptr;
	pe_base* LiteLib_PE = nullptr;
	export_info                OriginalBDS_ExportInf;
	exported_functions_list         OriginalBDS_ExportFunc;
	uint16_t                  ExportLimit = 0;
	int                       ExportCount = 1;
	int                       ApiDefCount = 1;
	int                       ApiDefFileCount = 1;
	
	std::ofstream BDSDef_API;
	std::ofstream BDSDef_VAR;
	std::ofstream BDSSymList;
	if (mGenModBDS) {
		OriginalBDS.open(mExeFile, std::ios::in | std::ios::binary);

		if (mOutputExeFile != "bedrock_server_mod.exe") {
			ProcessLibFile("LiteLoader.dll", mOutputExeFile);
			ProcessLibDirectory("plugins", mOutputExeFile);
			ProcessLibDirectory("plugins\\LiteLoader", mOutputExeFile);
		}

		if (OriginalBDS) {
			ModifiedBDS.open(mOutputExeFile, std::ios::out | std::ios::binary | std::ios::trunc);
			if (!ModifiedBDS) {
				std::cout << "[Err] Cannot create " << mOutputExeFile << std::endl;
				Pause(mPause);
				return -1;
			}
			if (!OriginalBDS) {
				std::cout << "[Err] Cannot open bedrock_server.exe" << std::endl;
				Pause(mPause);
				return -1;
			}
			OriginalBDS_PE = new pe_base(pe_factory::create_pe(OriginalBDS));
			try {
				OriginalBDS_ExportFunc = get_exported_functions(*OriginalBDS_PE, OriginalBDS_ExportInf);
			}
			catch (const pe_exception& e) {
				std::cout << "[Err] Get Exported Failed: " << e.what() << std::endl;
				Pause(mPause);
				return -1;
			}
			ExportLimit = get_export_ordinal_limits(OriginalBDS_ExportFunc).second;
		}
		else {
			std::cout << "[Err] Failed to Open bedrock_server.exe" << std::endl;
			Pause(mPause);
			return -1;
		}
	}
	if (mGenDevDef) {
		BDSDef_API.open(mDefApiFile, std::ios::ate | std::ios::out);
		if (!BDSDef_API) {
			std::cout << "[Err] Cannot create bedrock_server_api_0.def" << std::endl;
			Pause(mPause);
			return -1;
		}
		BDSDef_API << "LIBRARY bedrock_server.dll\nEXPORTS\n";

		BDSDef_VAR.open(mDefVarFile, std::ios::ate | std::ios::out);
		if (!BDSDef_VAR) {
			std::cout << "[Err] Cannot create bedrock_server_var.def" << std::endl;
			Pause(mPause);
			return -1;
		}
		BDSDef_VAR << "LIBRARY " << mOutputExeFile << "\nEXPORTS\n";
	}
	if (mGenSymbolList) {
		BDSSymList.open(mSymFile, std::ios::ate | std::ios::out);
		if (!BDSSymList) {
			std::cout << "[Err] Cannot create " << mSymFile << std::endl;
			Pause(mPause);
			return -1;
		}
	}
	for (const auto& fn : *FunctionList) {

		try {

			if (mGenSymbolList) {
				char tmp[11];
				sprintf_s(tmp, 11, "[%08d]", fn.Rva);
				
				auto demangledName = llvm::microsoftDemangle(
					fn.Name.c_str(), nullptr, nullptr, nullptr, nullptr,
					(llvm::MSDemangleFlags)(llvm::MSDF_NoCallingConvention | llvm::MSDF_NoAccessSpecifier));
				if (demangledName) {
					BDSSymList << demangledName << std::endl;
					free(demangledName);
				}
				BDSSymList << tmp << fn.Name << std::endl << std::endl;
			}
			bool skip = false;
			if (fn.Name[0] != '?') {
				skip = true;
				goto Skipped;
			}
			for (const auto& a : LLTool::SkipPerfix) {
				if (fn.Name.starts_with(a)) {
					skip = true;
					goto Skipped;
				}
			}
			for (const auto& reg : LLTool::SkipRegex) {
				std::smatch result;
				if (std::regex_match(fn.Name, result, reg)) {
					skip = true;
					goto Skipped;
				}
			}
		Skipped:

			if (mGenDevDef && !skip)
				if (fn.IsFunction) {
					BDSDef_API << "\t" << fn.Name << "\n";
				}
				else
					BDSDef_VAR << "\t" << fn.Name << "\n";
			if (mGenModBDS && !fn.IsFunction && !skip) {
				exported_function func;
				func.set_name(fn.Name);
				func.set_rva(fn.Rva);
				func.set_ordinal(ExportLimit + ExportCount);
				ExportCount++;
				if (ExportCount > 65535) {
					std::cout << "[Err] Too many Symbols are going to insert to ExportTable" << std::endl;
					Pause(mPause);
					return 1;
				}
				OriginalBDS_ExportFunc.push_back(func);
			}
		}
		catch (const pe_exception& e) {
			std::cout << "PeEditor : " << e.what() << std::endl;
			Pause(mPause);
			return -1;
		}
		catch (const std::regex_error& e) {
			std::cout << "RegexErr : " << e.what() << std::endl;
			Pause(mPause);
			return -1;
		}
		catch (...) {
			std::cout << "UnkErr " << std::endl;
			Pause(mPause);
			return -1;
		}
	}
	if (mGenModBDS) {
		try {
			section ExportSection;
			ExportSection.get_raw_data().resize(1);
			ExportSection.set_name("ExpFunc");
			ExportSection.readable(true);
			section& attachedExportedSection = OriginalBDS_PE->add_section(ExportSection);
			rebuild_exports(*OriginalBDS_PE, OriginalBDS_ExportInf, OriginalBDS_ExportFunc, attachedExportedSection);


			imported_functions_list imports(get_imported_functions(*OriginalBDS_PE));

			import_library preLoader;
			preLoader.set_name("LLPreLoader.dll");

			imported_function func;
			func.set_name("dlsym_real");
			func.set_iat_va(0x1);

			preLoader.add_import(func);
			imports.push_back(preLoader);

			section ImportSection;
			ImportSection.get_raw_data().resize(1);
			ImportSection.set_name("ImpFunc");
			ImportSection.readable(true).writeable(true);
			section& attachedImportedSection = OriginalBDS_PE->add_section(ImportSection);
			rebuild_imports(*OriginalBDS_PE, imports, attachedImportedSection, import_rebuilder_settings(true, false));



			rebuild_pe(*OriginalBDS_PE, ModifiedBDS);
			ModifiedBDS.close();
			std::cout << "[Info] " << mOutputExeFile << " Created." << std::endl;
		}
		catch (pe_exception e) {
			std::cout << "[Error] Failed to rebuild " << mOutputExeFile << std::endl;
			std::cout << "[Error] " << e.what() << std::endl;
			ModifiedBDS.close();
			std::filesystem::remove(std::filesystem::path(mOutputExeFile));
		}
		catch (...) {
			std::cout << "[Error] Failed to rebuild " << mOutputExeFile << " with unk err" << std::endl;
			ModifiedBDS.close();
			std::filesystem::remove(std::filesystem::path(mOutputExeFile));
		}
	}
	if (mGenDevDef) {
		try {
			BDSDef_API.flush();
			BDSDef_API.close();
			BDSDef_VAR.flush();
			BDSDef_VAR.close();
			std::cout << "[Info] [DEV]bedrock_server_var/api def    Created" << std::endl;
		}
		catch (...) {
			std::cout << "[Error] Failed to Cerate bedrock_server_var/api.def" << std::endl;
		}
	}
	if (mGenSymbolList) {
		try {
			BDSSymList.flush();
			BDSSymList.close();
			std::cout << "[Info] [DEV]Symbol List File              Created" << std::endl;
		}
		catch (...) {
			std::cout << "[Error] Failed to Create " << mSymFile << std::endl;
		}
	}

	Pause(mPause);
	return 0;
}
