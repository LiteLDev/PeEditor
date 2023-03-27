# PeEditor

_part of LiteLoaderBDS Toolchain_

## Features

1. Generate modified BDS executable file for LiteLoaderBDS
2. Generate import library files for LiteLoaderBDS and Plugins
3. Generate .def files and a symbol list for looking up symbol names with corresponding RVA

## Usages

### 1. Double Click

This will automatically load the bedrock_server.exe and bedrock_server.pdb files from the current working directory, and proceed to create a modified version of the server called bedrock_server_mod.exe.


### 2. Use a Command Line

```
Usage:
  PeEditor [OPTION...]

  -m, --mod              Generate bedrock_server_mod.exe (will be true if no arg passed)
  -p, --pause            Pause before exit (will be true if no arg passed)
  -n, --new              Use LiteLoader v3 preview mode
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

## What's New
1. Merged LibraryBuilder into PeEditor
2. Added more options
3. Added support for upcoming LiteLoader v3
4. Improve process speed
