This is your extension source directory.

# Compiling
To compile an extension you can type: `./compile.sh <extension.cpp>`

Compiled extensions can be found in the `./compiled` directory. If you would like to auto install them after compiling use the `--install` switch:

`./compile.sh --install sysinfo.cpp`

You can list multiple extensions to compile at the same time. If you do not provide any extensions to compile, it will use `*.cpp` as the input. (Effectively compiling all extensions.)

The `--flag` switch will allow you to set compiler arguments:

`./compile.sh --install --flag required_file.cpp --flag -lcurl extension.cpp`
