add_rules("mode.debug", "mode.release")

set_policy("package.requires_lock", true)
add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires("llvm-prebuilt 18.1.1")
add_requires("raw_pdb")
add_requires("cxxopts")
add_requires("fmt")
add_requires("spdlog")
add_requires("ctre")

add_requires("demangler")
add_requires("pe_bliss")

set_runtimes("MD")

target("PeEditor")
    set_kind("binary")
    set_languages("c++20")
    set_symbols("debug")
    add_files("src/**.cpp")
    add_includedirs("src")
    add_cxflags("/utf-8", "/EHa")
    add_defines("UNICODE", "PE_EDITOR_VERSION=\"v3.4.1\"")
    add_syslinks("wsock32", "comdlg32")
    add_packages("llvm-prebuilt", "raw_pdb", "pe_bliss", "cxxopts", "fmt", "spdlog", "ctre", "demangler")
