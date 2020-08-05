#!/bin/bash
./clean.sh
cd ./build
rm -rf ./*
cmake ..
make -j$(shell grep -c ^processor /proc/cpuinfo 2>/dev/null)
cd ..
cd ./bin
mkdir Containers Recipes
cd ..
cp config.json ./bin
cp -r ./key ./bin/
cp ./lib/pow_enclave.signed.so ./bin
cp ./lib/km_enclave.signed.so ./bin
cp test.sh ./bin