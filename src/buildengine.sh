#!/bin/bash

if [ ! -d o ]; then
    mkdir o
fi

source ".hmEngineBuildNo.sh"

let "hmEngineBuild++"
echo "hmEngineBuild=${hmEngineBuild}">.hmEngineBuildNo.sh
echo "#define VERSION \"halfMod v0.1.0-build${hmEngineBuild}\"">include/.hmEngineBuild.h

err=0

while [[ $1 == --* ]]; do
    case "$1" in
        --build)
            g++ -std=c++11 -I "include" -o o/${2%.*}.o -c ${2} -fPIC
            shift 1
        ;;
    esac
    err=$?
    if [[ $err -ne 0 ]]; then
        exit $err
    fi
    shift 1
done

g++ -std=c++11 -I "include" -o ../halfmod_engine o/str_tok.o o/halfmod.o o/halfmod_func.o halfmod_engine.cpp -ldl

# only increase build number if it compiled.
if [[ $? != 0 ]]; then
	let "hmEngineBuild--"
	echo "hmEngineBuild=${hmEngineBuild}">.hmEngineBuildNo.sh
	echo "#define VERSION \"halfMod v0.1.0-build${hmEngineBuild}\"">include/.hmEngineBuild.h
fi

