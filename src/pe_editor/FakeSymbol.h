#pragma once

#include <optional>
#include <string>

namespace pe_editor::FakeSymbol {

// generate fakeSymbol for virtual functions
std::optional<std::string> getFakeSymbol(const std::string& fn, bool removeVirtual = false);

} // namespace pe_editor::FakeSymbol
