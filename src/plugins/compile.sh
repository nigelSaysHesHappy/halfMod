#!/bin/bash

if [ ! -d compiled ]; then
	mkdir compiled
fi

if [ ! -d ../../halfMod ]; then
    mkdir ../../halfMod
    if [ ! -d ../../halfMod/plugins ]; then
        mkdir ../../halfMod/plugins
    fi
fi

if [[ "$1" == "--install" ]]; then
	switchInstall=1
	shift 1
fi

if [[ "$1" == "" ]]; then
    set -- *.cpp
fi

if [ ! -f "$1" ]; then
    echo "Nothing to compile . . ."
    exit 1
fi

while [ -f "$1" ]; do
    echo "Compiling '$1' . . ."
	pluginsrc="$1"
	pluginhmo="compiled/${pluginsrc%.*}.hmo"
	g++ -std=c++11 -I ../include -o "${pluginhmo}" ../o/halfmod.o ../o/str_tok.o -shared "${pluginsrc}" -fPIC
	err=$?
	if [[ "$err" == "0" ]]; then
		if [[ "$switchInstall" == "1" ]]; then
			cp "${pluginhmo}" "../../halfMod/plugins/${pluginsrc%.*}.hmo"
			if [[ $? == 0 ]]; then
				echo "Installed . . ."
			fi
		fi
	fi
	if [[ $err -ne 0 ]]; then
    	echo "Error compiling '${pluginsrc}': $err"
	fi
	shift 1
done

