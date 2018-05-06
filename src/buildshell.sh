#!/bin/bash

if [ ! -d o ]; then
    mkdir o
fi

hsBuild=0

if [ -f ".hsBuildNo.sh" ]; then
    source ".hsBuildNo.sh"
fi

let "hsBuild++"
echo "hsBuild=${hsBuild}">.hsBuildNo.sh
echo "#define VERSION \"halfShell v0.3.3-build${hsBuild}\"">include/.hsBuild.h

#g++ -std=c++11 -stdlib=libc++ -I "include" -o ../halfshell halfshell/halfshell.cpp o/nigsock.o o/str_tok.o -pthread
/usr/bin/clang++-3.8 -std=c++11 -stdlib=libc++ -I "include" -o ../halfshell halfshell/halfshell.cpp halfshell/halfshell_func.cpp o/nigsock.o o/str_tok.o -pthread

# only increase build number if it compiled.
if [[ $? != 0 ]]; then
	let "hsBuild--"
	echo "hsBuild=${hsBuild}">.hsBuildNo.sh
	echo "#define VERSION \"halfShell v0.3.3-build${hsBuild}\"">include/.hsBuild.h
fi
