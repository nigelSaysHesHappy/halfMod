#!/bin/bash

chmod +x buildapi.sh
chmod +x buildengine.sh
chmod +x buildshell.sh
chmod +x plugins/compile.sh
chmod +x extensions/compile.sh
chmod +x ../server.sh
echo "Compiling halfShell . . ."
./buildshell.sh
err=$?
if [[ $err -ne 0 ]]; then
    exit $err
fi
echo "Compiling halfMod API . . ."
./buildapi.sh
err=$?
if [[ $err -ne 0 ]]; then
    exit $err
fi
echo "Compiling halfMod Engine . . ."
./buildengine.sh --build str_tok.cpp --build halfmod_func.cpp
err=$?
if [[ $err -ne 0 ]]; then
    exit $err
fi
if [[ "$1" == "--plugins" ]]; then
    if [ ! -d plugins ] || [ ! -f plugins/compile.sh ]; then
        mkdir plugins
        echo "Error: Missing './plugins/compile.sh' . . ."
        exit 1
    fi
    echo "Compiling plugins . . ."
    cd plugins
    ./compile.sh --install
    cd ..
    if [ ! -d extensions ] || [ ! -f extensions/compile.sh ]; then
        mkdir extensions
        echo "Error: Missing './extensions/compile.sh' . . ."
        exit 1
    fi
    echo "Compiling extensions . . ."
    cd extensions
    ./compile.sh --install
fi
echo "Done."

