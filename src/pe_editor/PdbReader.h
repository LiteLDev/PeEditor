#pragma once

#include <deque>
#include <memory>
#include <string>
#include <utility>

struct PdbSymbol {
    std::string name;
    uint32_t    rva;
    bool        isFunction;
    PdbSymbol(std::string_view name, uint32_t rva, bool isFunction) : name(name), rva(rva), isFunction(isFunction) {}
};

std::unique_ptr<std::deque<PdbSymbol>> loadPDB(const wchar_t* pdbPath);
