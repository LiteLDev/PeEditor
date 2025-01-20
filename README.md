# PeEditor

A tool to inject PreLoader to Minecraft Bedrock Dedicated Server

This utility provides features including generating modified BDS executable file for LeviLamina, generating import library files for LeviLamina and Plugins, generating .def files and a symbol list for looking up symbol names with corresponding RVA.

Due to the requirements of the copyright holder, this software has been closed-source since v3.8.0, if you have any problems, feel free to file an issue.

## Install

```sh
lip install github.com/LiteLDev/PeEditor
```

## Usage

```text
Usage:
  PeEditor [OPTION...]

  -m, --mod              Generate bedrock_server_mod.exe (will be true if no arg passed)
  -p, --pause            Pause before exit (will be true if no arg passed)
  -O, --old              Use old mode for LiteLoaderBDS
  -b, --bak              Add a suffix ".bak" to original server exe (will be true if no arg passed)
  -d, --def              Generate def files for develop propose
  -l, --lib              Generate lib files for develop propose
  -s, --sym              Generate symbol list containing symbol and rva
  -o, --output-dir arg   Output dir (default: ./)
      --exe arg          BDS executable file name (default: ./bedrock_server.exe)
      --pdb arg          BDS debug database file name (default: ./bedrock_server.pdb)
  -c, --choose-pdb-file  Choose PDB file with a window
  -v, --verbose          Verbose output
  -V, --version          Print version
  -h, --help             Print usage
```

## Used Projects

| Project       | License                         | Link                                                             |
| ------------- | ------------------------------- | ---------------------------------------------------------------- |
| pe_bliss      | Special                         | <https://code.google.com/archive/p/portable-executable-library/> |
| llvm          | Apache 2.0 with LLVM Exceptions | <https://github.com/llvm/llvm-project>                           |
| cxxopts       | MIT                             | <https://github.com/jarro2783/cxxopts>                           |
| fmt           | MIT                             | <https://github.com/fmtlib/fmt>                                  |
| nlohmann_json | MIT                             | <https://github.com/nlohmann/json>                               |
| spdlog        | MIT                             | <https://github.com/gabime/spdlog>                               |

## License

Copyright © 2024 LeviMC, All rights reserved.

Refer to EULA for more information.
