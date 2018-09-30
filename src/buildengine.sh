#!/bin/bash

if [ ! -d o ]; then
    mkdir o
fi

hmEngineBuild=0

if [ -f ".hmEngineBuildNo.sh" ]; then
    source ".hmEngineBuildNo.sh"
fi

let "hmEngineBuild++"
echo "#define VERSION \"halfMod v0.1.7-build${hmEngineBuild}\"">include/.hmEngineBuild.h

err=0

while [[ $1 == --* ]]; do
    case "$1" in
        --build)
#            g++ -std=c++11 -stdlib=libc++ -I "include" -o o/${2%.*}.o -c ${2} -fPIC
             /usr/bin/clang++-3.8 -std=c++11 -stdlib=libc++ -I "include" -o o/${2%.*}.o -c ${2} -fPIC
            shift 1
        ;;
    esac
    err=$?
    if [[ $err -ne 0 ]]; then
        exit $err
    fi
    shift 1
done

#g++ -std=c++11 -stdlib=libc++ -I "include" -o ../halfmod_engine o/str_tok.o o/halfmod.o o/halfmod_func.o halfmod_engine.cpp -ldl
/usr/bin/clang++-3.8 -std=c++11 -stdlib=libc++ -I "include" -o ../halfmod_engine o/str_tok.o o/halfmod.o o/halfmod_func.o halfmod_engine.cpp -ldl

# only increase build number if it compiled.
if [[ $? != 0 ]]; then
	let "hmEngineBuild--"
	echo "#define VERSION \"halfMod v0.1.7-build${hmEngineBuild}\"">include/.hmEngineBuild.h
else
    echo "hmEngineBuild=${hmEngineBuild}">.hmEngineBuildNo.sh
fi

