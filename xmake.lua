add_rules("mode.debug", "mode.release")

add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires("cxxopts")
add_requires("fmt")
add_requires("spdlog", {configs = {header_only = false}})

add_requires("pe_bliss")

set_runtimes("MD")

local version = "v3.9.0"

target("PeEditor")
    set_kind("binary")
    set_languages("c++20")
    set_symbols("debug")
    add_files("src/**.cpp")
    add_includedirs("src")
    add_cxflags("/utf-8", "/EHa")
    set_exceptions("none")
    add_defines("UNICODE", "PE_EDITOR_VERSION=\"" .. version .. "\"", "_CRT_NONSTDC_NO_DEPRECATE")
    add_syslinks("wsock32", "comdlg32")
    add_packages(
        "cxxopts", "fmt", "spdlog", "pe_bliss"
    )
