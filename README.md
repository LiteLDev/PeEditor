# PeEditor

_part of LeviLamina Toolchain_

## Features

1. Generate modified BDS executable file for LeviLamina
2. Generate import library files for LeviLamina and Plugins
3. Generate .def files and a symbol list for looking up symbol names with corresponding RVA

## Usages

### 1. Double Click

This will automatically load the bedrock_server.exe and bedrock_server.pdb files from the current working directory, and proceed to create a modified version of the server called bedrock_server_mod.exe.


### 2. Use a Command Line

```
LeviLamina ToolChain PeEditor v3.4.1
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

## What's New
1. Merged LibraryBuilder into PeEditor
2. Added more options
3. Improve process speed
