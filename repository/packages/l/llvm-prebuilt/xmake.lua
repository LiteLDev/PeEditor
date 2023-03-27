package("llvm-prebuilt")
    set_homepage("https://llvm.org/")
    set_description("The LLVM Project is a collection of modular and reusable compiler and toolchain technologies.")
    set_license("Apache-2.0")

    add_urls("https://github.com/awakecoding/llvm-prebuilt/releases/download/v2023.1.0/clang+llvm-14.0.6-x86_64-windows.tar.xz")

    add_versions("14.0.6", "9eeed63df75e90a3b1e67ad574c954d8c215e879881e891b4e1c83df09d68056")

    on_install("windows", function (package)
        if not is_arch("x64") then
            raise("only x64 supported")
        end
        os.mv("*", package:installdir())
    end)
