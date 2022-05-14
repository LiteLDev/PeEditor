#pragma warning(disable: 4996)
#pragma warning(disable: 4146)
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#pragma warning(disable: 4624)

#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdlib>

#include "llvm/Object/COFF.h"
#include "llvm/Object/COFFImportFile.h"
#include "llvm/Object/COFFModuleDefinition.h"
#include "../LLPeEditor/pdb.h"
#include "../LLPeEditor/skipFunctions.h"

#include <windows.h>
#include <ShlObj.h>

using namespace llvm;
using namespace llvm::object;
using namespace llvm::COFF;

using namespace std;

string wstr2str(wstring wstr) {
    auto  len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    char* buffer = new char[len + 1];
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer, len + 1, NULL, NULL);
    buffer[len] = '\0';

    string result = string(buffer);
    delete[] buffer;
    return result;
}

std::wstring str2wstr(const std::string& str, UINT codePage)
{
    auto len = MultiByteToWideChar(codePage, 0, str.c_str(), -1, nullptr, 0);
    auto* buffer = new wchar_t[len + 1];
    MultiByteToWideChar(codePage, 0, str.c_str(), -1, buffer, len + 1);
    buffer[len] = L'\0';

    wstring result = wstring(buffer);
    delete[] buffer;
    return result;
}

wstring str2wstr(const string& str)
{
    return str2wstr(str, CP_UTF8);
}

string ChooseFolder()
{
    TCHAR szBuffer[MAX_PATH] = { 0 };

    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner = NULL;
    bi.pszDisplayName = szBuffer;
    bi.lpszTitle = L"Choose a folder that contains bedrock_server.pdb";
    bi.ulFlags = BIF_RETURNFSANCESTORS;

    LPITEMIDLIST idl = SHBrowseForFolder(&bi);
    if (idl == NULL)
    {
        return "";
    }
    SHGetPathFromIDList(idl, szBuffer);
    return wstr2str(szBuffer);
}

void ErrorExit(int code)
{
    system("pause");
    exit(code);
}

std::string TCHAR2STRING(TCHAR* STR)
{
    int iLen = WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);
    char* chRtn = new char[iLen * sizeof(char)];
    WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);
    std::string str(chRtn);
    return str;
}

string& ReplaceStr(string& str, const string& old_value, const string& new_value)
{
    for (string::size_type pos(0); pos != string::npos; pos += new_value.length())
    {
        if ((pos = str.find(old_value, pos)) != string::npos)
            str.replace(pos, old_value.length(), new_value);
        else
            break;
    }
    return str;
}


int main(int argc, char** argv)
{
    //Welcome
    cout << "\n";
    cout << "  /////////////////////////////////////////////////////\n";
    cout << "  //                                                 //\n";
    cout << "  //         LiteLoader Dev Library Builder          //\n";
    cout << "  //                                                 //\n";
    cout << "  /////////////////////////////////////////////////////\n";
    cout << "\n---Start Processing..." << endl;

    string bdsPath;
    string generatedPath("../LiteLoader/Lib");

    TCHAR a[MAX_PATH];
    memset(a, 0, MAX_PATH);
    GetModuleFileName(NULL, a, MAX_PATH);
    string exeRunPath = TCHAR2STRING(a);
    ReplaceStr(exeRunPath, "\\LibraryBuilder.exe", "");
    if (argc == 1 || ((argc == 3 || argc == 4) && string(argv[1]) == "-o"))
    {
        if (argc >= 3)
            generatedPath = string(argv[2]);
        if (argc == 4)
            bdsPath = string(argv[3]);
        else
        {
            MessageBox(NULL, L"Extra static library is needed to finish this compile.\n"
                "You need to choose a folder contains bedrock_server.exe and bedrock_server.pdb next",
                L"More action needed", MB_OK | MB_ICONINFORMATION);

            // Choose Folder
            bdsPath = ChooseFolder();
        }
    }
    else if (argc == 2)
        bdsPath = string(argv[1]);

    // Check Valid
    if (bdsPath.empty())
    {
        MessageBox(NULL, L"You must select a folder for the compilation process to work properly", L"Error", MB_OK | MB_ICONERROR);
        cout << "\n**** [Error]\n"
            "**** You must choose a folder for process!" << endl;
        ErrorExit(-1);
    }

    else if (!filesystem::exists(bdsPath + "/bedrock_server.pdb"))
    {
        MessageBox(NULL, L"bedrock_server.pdb was not found in the folder you provided", L"Error", MB_OK | MB_ICONERROR);
        cout << "\n**** [Error]\n"
            "**** You must choose a folder contains *bedrock_server.pdb*!" << endl;
        ErrorExit(-3);
    }
    filesystem::current_path(exeRunPath);
	
    //Running process
    cout << "\n---- Loading bedrock_server.pdb ..." << endl;

    auto pdbData = loadPDB(str2wstr(bdsPath + "/bedrock_server.pdb").c_str());

    std::vector<COFFShortExport> ApiExports;
    std::vector<COFFShortExport> VarExports;

	for (auto const& i : *pdbData) {
        if (i.Name[0] != '?') {
            continue;
        }
		
		if ([&]() {
			for (const auto& a : LLTool::SkipPerfix) {
				if (i.Name.starts_with(a)) {
					return true;
				}
			}
			return false;
			}()) {
			continue;
		}

		if ([&]() {
			for (const auto& reg : LLTool::SkipRegex) {
				std::smatch result;
					if (std::regex_match(i.Name, result, reg)) {
						return true;
					}
			}
			return false;
			}()) {
			continue;
		}


		COFFShortExport record;
		record.Name = i.Name;
		if (i.IsFunction)
			ApiExports.push_back(record);
		else
			VarExports.push_back(record);
	}
	
    cout << "\n---- Generating Libs..." << endl;
	
	//prevent Marco IMAGE_FILE_MACHINE_AMD64 expansion
#pragma push_macro("IMAGE_FILE_MACHINE_AMD64")
#undef IMAGE_FILE_MACHINE_AMD64
    auto apiFail = writeImportLibrary("bedrock_server.dll", generatedPath + "/bedrock_server_api.lib", ApiExports, COFF::IMAGE_FILE_MACHINE_AMD64, true);
    auto varFail = writeImportLibrary("bedrock_server_mod.exe", generatedPath + "/bedrock_server_var.lib", VarExports, COFF::IMAGE_FILE_MACHINE_AMD64, true);

#pragma pop_macro("IMAGE_FILE_MACHINE_AMD64")

    // Check valid
    if (apiFail)
    {
        MessageBox(NULL, L"Fail to generate import library for apis!", L"Error", MB_OK | MB_ICONERROR);
        cout << "\n**** [Error]\n"
            "**** Fail to generate import library for apis!" << endl;
        ErrorExit(-4);
    }
    if (varFail)
    {
        MessageBox(NULL, L"Fail to generate import library for variables!", L"Error", MB_OK | MB_ICONERROR);
        cout << "\n**** [Error]\n"
            "**** Fail to generate import library for variables!" << endl;
        ErrorExit(-5);
    }

    cout << "\n---- [Success]" << endl;
    cout << "---- Everything is OK." << endl;
    Sleep(3000);
    return 0;
}