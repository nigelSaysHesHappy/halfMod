#!/bin/bash

if [ ! -d o ]; then
    mkdir o
fi

hmAPIBuild=0

if [ -f ".hmAPIBuildNo.sh" ]; then
    source ".hmAPIBuildNo.sh"
fi

let "hmAPIBuild++"
echo "hmAPIBuild=${hmAPIBuild}">.hmAPIBuildNo.sh
echo "#define API_VERSION \"v0.2.5-build${hmAPIBuild}\"">include/.hmAPIBuild.h

#g++ -std=c++11 -stdlib=libc++ -I "include" -o o/halfmod.o -c api/halfmod.cpp -ldl -fPIC
/usr/bin/clang++-3.8 -std=c++11 -stdlib=libc++ -I "include" -o o/halfmod.o -c api/halfmod.cpp -ldl -fPIC

# only increase build number if it compiled.
if [[ $? != 0 ]]; then
	let "hmAPIBuild--"
	echo "hmAPIBuild=${hmAPIBuild}">.hmAPIBuildNo.sh
	echo "#define API_VERSION \"v0.2.5-build${hmAPIBuild}\"">include/.hmAPIBuild.h
fi
