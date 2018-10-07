#!/bin/bash

source "usePCRE2.sh"

if [ ! -d o ]; then
    mkdir o
fi

hmAPIBuild=0

if [ -f ".hmAPIBuildNo.sh" ]; then
    source ".hmAPIBuildNo.sh"
fi

let "hmAPIBuild++"
echo "#define API_VERSION \"v0.2.8-build${hmAPIBuild}\"">include/.hmAPIBuild.h
if $usePCRE; then
	echo "#define HM_USE_PCRE2">>include/.hmAPIBuild.h
fi

#g++ -std=c++11 -stdlib=libc++ -I "include" -o o/halfmod.o -c api/halfmod.cpp -ldl -fPIC
clang++ -std=c++11 -stdlib=libc++ -I "include" -o o/halfmod.o -c api/halfmod.cpp -fPIC
if $usePCRE; then
	clang++ -std=c++11 -stdlib=libc++ -I "include" -o o/pcre2_halfwrap.o -c pcre2_halfwrap.cpp -fPIC
fi

# only increase build number if it compiled.
if [[ $? != 0 ]]; then
	let "hmAPIBuild--"
	echo "#define API_VERSION \"v0.2.8-build${hmAPIBuild}\"">include/.hmAPIBuild.h
	if $usePCRE; then
		echo "#define HM_USE_PCRE2">>include/.hmAPIBuild.h
    fi
else
    echo "hmAPIBuild=${hmAPIBuild}">.hmAPIBuildNo.sh
fi
