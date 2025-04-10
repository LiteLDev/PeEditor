#pragma once

#include <memory>
#include <string>

#include "cxxopts.hpp"

class CxxOptsAdder {
    cxxopts::Options&    options;
    cxxopts::OptionAdder adder;

public:
    inline CxxOptsAdder(cxxopts::Options& options, const std::string& groupName = "")
    : options(options),
      adder(options.add_options(groupName)) {}

    inline CxxOptsAdder&
    add(const std::string&                           opts,
        const std::string&                           desc,
        const std::shared_ptr<const cxxopts::Value>& value    = ::cxxopts::value<bool>(),
        std::string                                  arg_help = "") {
        adder(opts, desc, value, arg_help);
        return *this;
    }
};
