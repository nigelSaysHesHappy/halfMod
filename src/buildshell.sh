#!/bin/bash

if [ ! -d o ]; then
    mkdir o
fi

hsBuild=0

if [ -f ".hsBuildNo.sh" ]; then
    source ".hsBuildNo.sh"
fi

let "hsBuild++"
echo "#define VERSION \"halfShell v0.4.1-build${hsBuild}\"">include/.hsBuild.h

g++ -std=c++11 -I "include" -o ../halfshell halfshell/halfshell.cpp -shared -fPIC
#/usr/bin/clang++-3.8 -std=c++11 -stdlib=libc++ -I "include" -o ../halfshell halfshell/halfshell.cpp halfshell/halfshell_func.cpp o/nigsock.o o/str_tok.o -pthread

# only increase build number if it compiled.
if [[ $? != 0 ]]; then
	let "hsBuild--"
	echo "#define VERSION \"halfShell v0.4.1-build${hsBuild}\"">include/.hsBuild.h
else
    echo "hsBuild=${hsBuild}">.hsBuildNo.sh
fi
