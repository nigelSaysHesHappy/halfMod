#!/bin/bash

source "../usePCRE2.sh"
if $usePCRE; then
    pcrelib="../o/pcre2_halfwrap.o -lpcre2-8"
fi

if [ ! -d compiled ]; then
	mkdir compiled
fi

if [ ! -d ../../halfMod ]; then
    mkdir ../../halfMod
fi
if [ ! -d ../../halfMod/plugins ]; then
    mkdir ../../halfMod/plugins
fi

while [[ $1 == --* ]]; do

	if [[ "$1" == "--install" ]]; then
		switchInstall=1
	elif [[ "$1" == "--flag" ]]; then
		flags+=( `eval echo '$2'` )
		shift 1
	fi
	shift 1
done

echo "${flags[@]}"

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
#	g++ -std=c++11 -stdlib=libc++ -I ../include -o "${pluginhmo}" ../o/halfmod.o ../o/str_tok.o -shared "${pluginsrc}" -fPIC
        /usr/bin/clang++-3.8 -std=c++11 -stdlib=libc++ -I ../include -o "${pluginhmo}" ../o/halfmod.o ../o/str_tok.o $pcrelib "${flags[@]}" -shared "${pluginsrc}" -fPIC
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

