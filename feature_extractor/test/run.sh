#!/bin/bash
~/Build/Popcorn_Autobench/install/gcc48.x86_64/bin/gcc -fplugin=../libfeature_extractor.so -c $*
cat *.features > ${1}.noprof.all
rm -f *.o *.features

~/Build/Popcorn_Autobench/install/gcc48.x86_64/bin/gcc -fprofile-generate $* -o tmp.bin
./tmp.bin
~/Build/Popcorn_Autobench/install/gcc48.x86_64/bin/gcc -fprofile-use -fprofile-correction -fplugin=../libfeature_extractor.so -c $*
cat *.features > ${1}.prof.all
rm -f *.o *.gcda *.features tmp.bin
