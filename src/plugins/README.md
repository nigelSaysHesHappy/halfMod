This is your plugin source directory.

# Compiling
To compile a plugin you can type: `./compile.sh <plugin.cpp>`

Compiled plugins can be found in the `./compiled` directory. If you would like to auto install them after compiling use the `--install` switch:

`./compile.sh --install admincmds.cpp`

You can list multiple plugins to compile at the same time. If you do not provide any plugins to compile, it will use `*.cpp` as the input. (Effectively compiling all plugins.)
