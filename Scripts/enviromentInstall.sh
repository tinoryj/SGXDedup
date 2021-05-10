#!/usr/bin/bash
sudo apt update
sudo apt dist-upgrade
sudo apt autoremove
sudo apt install build-essential cmake wget libssl-dev libcurl4-openssl-dev libprotobuf-dev libboost-all-dev libleveldb-dev libsnappy-dev
cd ./Packages/
cp FindSGX.cmake /usr/share/cmake-3.10/Modules/
chmod +x sgx_linux_x64_driver_2.6.0_4f5bb63.bin
sudo ./sgx_linux_x64_driver_2.6.0_4f5bb63.bin
sudo dpkg -i ./libsgx-enclave-common_2.7.100.4-bionic1_amd64.deb
chmod +x sgx_linux_x64_sdk_2.7.100.4.bin
sudo ./sgx_linux_x64_sdk_2.7.100.4.bin --prefix=/opt/intel
source /opt/intel/sgxsdk/environment
unzip intel-sgx-ssl-lin_2.5_1.1.1d.zip
cp openssl-1.1.1d.tar.gz intel-sgx-ssl-lin_2.5_1.1.1d/openssl_source/
cd intel-sgx-ssl-lin_2.5_1.1.1d/Linux/
make all
make test
sudo make install
cd /opt/intel/sgxssl/include
sudo mv pthread.h sgxpthread.h
sudo sed -i '415c #include \"sgxpthread.h\"' /opt/intel/sgxssl/include/openssl/crypto.h
echo "Environment set done"
