#pragma once

#include <deque>
#include <memory>
#include <string>
#include <utility>

struct PdbSymbol {
    std::string name;
    uint32_t    rva;
    bool        isFunction;
    bool        fromModule;
    bool        verbose;
    PdbSymbol(std::string name, uint32_t rva, bool isFunction, bool fromModule, bool verbose)
    : name(std::move(name)),
      rva(rva),
      isFunction(isFunction),
      fromModule(fromModule),
      verbose(verbose) {}
};

std::unique_ptr<std::deque<PdbSymbol>> loadPDB(const wchar_t* pdbPath);
