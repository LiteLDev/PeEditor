# PeEditor

__LiteLoaderBDS Toolchain__

## Features

1. Generate modded BDS executable file for LiteLoaderBDS
2. Generate library files for LiteLoaderBDS
3. Generate def files and symbol list for looking up symbol name and rva

## Usages

### 1. Directly Execute

Will auto load current working dir's bedrock_server.exe and bedrock_server.pdb, and create a bedrock_server_mod.exe.

### 2. With Argument

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
1. Merged LibraryBuilder to PeEditor
2. Added more options
3. Added LiteLoader3 mode
4. Improve process speed
