./clean.sh
cd ./build
rm -rf ./*
cmake ..
make -j6
cd ..
cd ./bin
mkdir cr fr kr tmp
cd ..
cp config.json ./bin
cp -r ./key ./bin/
cp ./lib/pow_enclave.signed.so ./bin
cp ./lib/km_enclave.signed.so ./bin
cp  wgetpost ./bin/tmp/