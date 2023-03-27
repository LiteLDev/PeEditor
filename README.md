# PeEditor

part of _LiteLoaderBDS Toolchain_

> I would like to assure you that our team will improve it to make it more clear and concise after the LL3 refactoring is completed. We welcome any feedback or suggestions you might have for the new branch.

## Features

1. Generate modded BDS executable file works with LiteLoaderBDS
2. Generate import library files for LiteLoaderBDS and Plugins
3. Generate .def files and  symbol list that can be used to lookup symbol names and their corresponding RVA (relative virtual address).

## Usages

### 1. Double click

This will automatically load the bedrock_server.exe and bedrock_server.pdb files from the current working directory, and proceed to create a modified version of the server called bedrock_server_mod.exe.


### 2. Use command line

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

## What's new
1. Merged LibraryBuilder into PeEditor
2. Added more options
3. Added support for upcoming LiteLoader v3
4. Improve process speed
