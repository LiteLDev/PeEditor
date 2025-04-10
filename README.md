# PeEditor

A tool to inject PreLoader to Minecraft Bedrock Dedicated Server

This utility provides features including generating modified BDS executable file for LeviLamina, generating import library files for LeviLamina and Plugins, generating .def files and a symbol list for looking up symbol names with corresponding RVA.

## Install

```sh
lip install github.com/LiteLDev/PeEditor
```

## Usage

```text
Usage:
  PeEditor [OPTION...]

  -m, --mod             Generate mod.exe (will be true if no arg passed)
  -p, --pause           Pause before exit (will be true if no arg passed)
  -b, --bak             Add a suffix ".bak" to original server exe (will be true if no arg passed)
      --inplace         name inplace
  -o, --output-dir arg  Output dir (default: ./)
      --exe arg         executable file name (default: ./bedrock_server.exe)
      --verbose         Verbose output
  -V, --version         Print version
  -h, --help            Print usage
```

## Used Projects

| Project  | License | Link                                                             |
|----------|---------|------------------------------------------------------------------|
| pe_bliss | Special | <https://code.google.com/archive/p/portable-executable-library/> |
| cxxopts  | MIT     | <https://github.com/jarro2783/cxxopts>                           |
| fmt      | MIT     | <https://github.com/fmtlib/fmt>                                  |
| spdlog   | MIT     | <https://github.com/gabime/spdlog>                               |

## Contributing

Ask questions by creating an issue.

PRs accepted.

## License

GPL-3.0-or-later © LiteLDev
