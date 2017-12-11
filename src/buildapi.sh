#!/bin/bash

if [ ! -d o ]; then
    mkdir o
fi

source ".hmAPIBuildNo.sh"

let "hmAPIBuild++"
echo "hmAPIBuild=${hmAPIBuild}">.hmAPIBuildNo.sh
echo "#define API_VERSION \"v0.2.0-build${hmAPIBuild}\"">include/.hmAPIBuild.h

g++ -std=c++11 -I "include" -o o/halfmod.o -c api/halfmod.cpp -ldl -fPIC

# only increase build number if it compiled.
if [[ $? != 0 ]]; then
	let "hmAPIBuild--"
	echo "hmAPIBuild=${hmAPIBuild}">.hmAPIBuildNo.sh
	echo "#define API_VERSION \"v0.2.0-build${hmAPIBuild}\"">include/.hmAPIBuild.h
fi
