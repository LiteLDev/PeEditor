#pragma once

#include <deque>
#include <memory>
#include <string>
#include <utility>

struct PdbSymbol {
    std::string name;
    uint32_t    rva;
    bool        isFunction;
    bool        isPublic;
    bool        verbose;
    PdbSymbol(std::string name, uint32_t rva, bool isFunction, bool isPublic, bool verbose)
    : name(std::move(name)),
      rva(rva),
      isFunction(isFunction),
      isPublic(isPublic),
      verbose(verbose) {}
};

std::unique_ptr<std::deque<PdbSymbol>> loadPDB(const wchar_t* pdbPath);
