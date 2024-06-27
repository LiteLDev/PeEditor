#include "pe_editor/PdbReader.h"

#include "PDB.h"
#include "PDB_DBIStream.h"
#include "PDB_InfoStream.h"
#include "PDB_RawFile.h"

#include "PeEditor.h"

#include <windows.h>

namespace MemoryMappedFile {
struct Handle {
    HANDLE file;
    HANDLE fileMapping;
    void*  baseAddress;
};

MemoryMappedFile::Handle Open(const wchar_t* path) {
    HANDLE file =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return Handle{INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, nullptr};
    }

    HANDLE fileMapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (fileMapping == nullptr) {
        CloseHandle(file);

        return Handle{INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, nullptr};
    }

    void* baseAddress = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
    if (baseAddress == nullptr) {
        CloseHandle(fileMapping);
        CloseHandle(file);

        return Handle{INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, nullptr};
    }

    return Handle{file, fileMapping, baseAddress};
}


void Close(Handle& handle) {
    UnmapViewOfFile(handle.baseAddress);
    CloseHandle(handle.fileMapping);
    CloseHandle(handle.file);

    handle.file        = nullptr;
    handle.fileMapping = nullptr;
    handle.baseAddress = nullptr;
}
} // namespace MemoryMappedFile
namespace {
inline bool IsError(PDB::ErrorCode errorCode) {
    using pe_editor::logger;
    switch (errorCode) {
    case PDB::ErrorCode::Success:
        return false;
    case PDB::ErrorCode::InvalidSuperBlock:
        logger->error("[PDB] Invalid SuperBlock");
        return true;
    case PDB::ErrorCode::InvalidFreeBlockMap:
        logger->error("[PDB] Invalid free block map");
        return true;
    case PDB::ErrorCode::InvalidSignature:
        logger->error("[PDB] Invalid signature");
        return true;
    case PDB::ErrorCode::InvalidStreamIndex:
        logger->error("[PDB] Invalid stream index");
        return true;
    case PDB::ErrorCode::UnknownVersion:
        logger->error("[PDB] Unknown version");
        return true;
    case PDB::ErrorCode::InvalidStream:
        logger->error("[PDB] Invalid stream");
        break;
    }
    return true;
}

inline bool HasValidDBIStreams(const PDB::RawFile& rawPdbFile, const PDB::DBIStream& dbiStream) {
    if (IsError(dbiStream.HasValidImageSectionStream(rawPdbFile))) return false;
    if (IsError(dbiStream.HasValidPublicSymbolStream(rawPdbFile))) return false;
    if (IsError(dbiStream.HasValidGlobalSymbolStream(rawPdbFile))) return false;
    if (IsError(dbiStream.HasValidSectionContributionStream(rawPdbFile))) return false;
    return true;
}
} // namespace

std::unique_ptr<std::deque<PdbSymbol>> loadFunctions(const PDB::RawFile& rawPdbFile, const PDB::DBIStream& dbiStream) {
    auto                          func               = std::make_unique<std::deque<PdbSymbol>>();
    const PDB::ImageSectionStream imageSectionStream = dbiStream.CreateImageSectionStream(rawPdbFile);
    const PDB::CoalescedMSFStream symbolRecordStream = dbiStream.CreateSymbolRecordStream(rawPdbFile);
    const PDB::PublicSymbolStream publicSymbolStream = dbiStream.CreatePublicSymbolStream(rawPdbFile);
    const PDB::ModuleInfoStream   moduleInfoStream   = dbiStream.CreateModuleInfoStream(rawPdbFile);

    const PDB::ArrayView<PDB::HashRecord> hashRecords = publicSymbolStream.GetRecords();

    for (const PDB::HashRecord& hashRecord : hashRecords) {
        const PDB::CodeView::DBI::Record* record = publicSymbolStream.GetRecord(symbolRecordStream, hashRecord);
        const uint32_t                    rva =
            imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_PUB32.section, record->data.S_PUB32.offset);
        if (rva == 0u) continue;
        const std::string symbol(record->data.S_PUB32.name);
        if (symbol.empty()) continue;
        const bool isFunction = (bool)(record->data.S_PUB32.flags & PDB::CodeView::DBI::PublicSymbolFlags::Function);
        func->push_back(PdbSymbol(symbol, rva, isFunction, false, false));
    }
    // module symbols are those symbols that are not visible to the linker
    // usually anonymous implementation/cleanup dtor/etc..., without mangling
    for (const PDB::ModuleInfoStream::Module& module : moduleInfoStream.GetModules()) {
        if (!module.HasSymbolStream()) {
            continue;
        }
        const PDB::ModuleSymbolStream moduleSymbolStream = module.CreateSymbolStream(rawPdbFile);
        moduleSymbolStream.ForEachSymbol([&imageSectionStream, &func](const PDB::CodeView::DBI::Record* record) {
            if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32) {
                uint32_t rva = imageSectionStream.ConvertSectionOffsetToRVA(
                    record->data.S_LPROC32.section,
                    record->data.S_LPROC32.offset
                );
                if (rva == 0u) return;

                std::string_view name = record->data.S_LPROC32.name;
                bool             skip = true;
                // symbol with '`' prefix is special symbol
                if (name.starts_with("`")) {
                    if (name.starts_with("`anonymous namespace'")) {
                        skip = false;
                    }
                } else {
                    if (name.starts_with("std::_Func_impl_no_alloc") || name.find("`dynamic") != std::string_view::npos)
                        skip = true;
                    else skip = false;
                }
                const bool isFunction =
                    (bool)(record->data.S_PUB32.flags & PDB::CodeView::DBI::PublicSymbolFlags::Function);
                func->push_back(PdbSymbol(record->data.S_LPROC32.name, rva, isFunction, true, skip));
            }
        });
    }
    return func;
}

std::unique_ptr<std::deque<PdbSymbol>> loadPDB(const wchar_t* pdbPath) {
    MemoryMappedFile::Handle pdbFile = MemoryMappedFile::Open(pdbPath);
    if (!pdbFile.baseAddress) {
        return nullptr;
    }
    if (IsError(PDB::ValidateFile(pdbFile.baseAddress))) {
        MemoryMappedFile::Close(pdbFile);
        return nullptr;
    }
    const PDB::RawFile rawPdbFile = PDB::CreateRawFile(pdbFile.baseAddress);
    if (IsError(PDB::HasValidDBIStream(rawPdbFile))) {
        MemoryMappedFile::Close(pdbFile);
        return nullptr;
    }
    const PDB::InfoStream infoStream(rawPdbFile);
    if (infoStream.UsesDebugFastLink()) {
        MemoryMappedFile::Close(pdbFile);
        return nullptr;
    }
    const PDB::DBIStream dbiStream = PDB::CreateDBIStream(rawPdbFile);
    if (!HasValidDBIStreams(rawPdbFile, dbiStream)) {
        MemoryMappedFile::Close(pdbFile);
        return nullptr;
    }
    return loadFunctions(rawPdbFile, dbiStream);
}
