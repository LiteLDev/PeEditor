add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires("cxxopts")
add_requires("fmt")
add_requires("spdlog", {configs = {header_only = false}})

add_requires("pe_bliss")

set_runtimes("MD")

target("PeEditor")
    set_kind("binary")
    set_languages("c++20")
    set_symbols("debug")
    add_files("src/**.cpp")
    add_includedirs("src")
    add_cxflags("/utf-8", "/EHa")
    set_exceptions("none")
    add_defines("UNICODE", "_CRT_NONSTDC_NO_DEPRECATE")
    add_syslinks("wsock32", "comdlg32")
    add_packages(
        "cxxopts", "fmt", "spdlog", "pe_bliss"
    )
    on_load(function(target)
        local tag = os.iorun("git describe --tags --abbrev=0 --always")
        local major, minor, patch, suffix = tag:match("v(%d+)%.(%d+)%.(%d+)(.*)")
        if not major then
            print("Failed to parse version tag, using 0.0.0")
            major, minor, patch = 0, 0, 0
        end
        target:add("defines", "PE_EDITOR_VERSION=\"" .. major .. "." .. minor .. "." .. patch .. "\"")
    end)
