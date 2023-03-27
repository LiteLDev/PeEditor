add_rules("mode.debug", "mode.release")

set_policy("package.requires_lock", true)
add_repositories("local-repo repository")
add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires("llvm-prebuilt")
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
    add_includedirs("include")
    add_cxflags("/utf-8")
    add_defines("UNICODE")
    add_syslinks("user32", "comdlg32")
    add_packages("llvm-prebuilt", "raw_pdb", "pe_bliss", "cxxopts", "fmt", "spdlog", "ctre", "demangler")
