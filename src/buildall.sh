#!/bin/bash

echo "Compiling halfMod API . . ."
./buildapi.sh
err=$?
if [[ $err -ne 0 ]]; then
    exit $err
fi
echo "Compiling halfMod Engine . . ."
./buildengine.sh --build str_tok.cpp --build nigsock.cpp --build halfmod_func.cpp
err=$?
if [[ $err -ne 0 ]]; then
    exit $err
fi
echo "Compiling halfShell . . ."
./buildshell.sh
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
fi
echo "Done."

