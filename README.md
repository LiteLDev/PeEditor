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

## Contributing

Ask questions by creating an issue.

PRs accepted.

## License

GPL-3.0-or-later © LiteLDev
