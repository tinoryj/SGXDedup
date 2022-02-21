#!/bin/bash
# sudo apt update
# sudo apt dist-upgrade
# sudo apt autoremove
if [ ! -d "Packages" ]; then
  mkdir Packages
fi

echo "Install system build prerequests"
sudo apt install -y build-essential cmake wget libssl-dev libcurl4-openssl-dev libprotobuf-dev libboost-all-dev libleveldb-dev libsnappy-dev
cd ./Packages/

echo "Download SGX Driver"
if [ ! -f "sgx_linux_x64_driver_1.41.bin" ]; then
  wget https://download.01.org/intel-sgx/sgx-linux/2.15.1/distro/ubuntu20.04-server/sgx_linux_x64_driver_1.41.bin
fi
if [ ! -f "sgx_linux_x64_driver_2.11.0_2d2b795.bin" ]; then
  wget https://download.01.org/intel-sgx/sgx-linux/2.15.1/distro/ubuntu20.04-server/sgx_linux_x64_driver_2.11.0_2d2b795.bin
fi

echo "Download SGX SDK"
if [ ! -f "sgx_linux_x64_sdk_2.15.101.1.bin" ]; then
  wget https://download.01.org/intel-sgx/sgx-linux/2.15.1/distro/ubuntu20.04-server/sgx_linux_x64_sdk_2.15.101.1.bin
fi

echo "Install SGX Driver & SDK"
targetPackageSet=('sgx_linux_x64_driver_1.41.bin' 'sgx_linux_x64_driver_2.11.0_2d2b795.bin' 'sgx_linux_x64_sdk_2.15.101.1.bin')
for filename in ${targetPackageSet[@]}; do
  if test -x $filename ; then
    ls -l $filename
  else
    chmod +x $filename
  fi
done
sudo ./sgx_linux_x64_driver_1.41.bin
sudo ./sgx_linux_x64_driver_2.11.0_2d2b795.bin
sudo ./sgx_linux_x64_sdk_2.15.101.1.bin --prefix=/opt/intel
source /opt/intel/sgxsdk/environment

echo "Install SGX runtime environment"
if [ ! -f "/etc/apt/sources.list.d/intel-sgx.list" ]; then
  echo 'deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu focal main' | sudo tee /etc/apt/sources.list.d/intel-sgx.list
fi
wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | sudo apt-key add
sudo apt install -y libsgx-epid libsgx-quote-ex libsgx-dcap-ql libsgx-enclave-common-dbgsym libsgx-dcap-ql-dbgsym libsgx-dcap-default-qpl-dbgsym libsgx-urts libsgx-launch libsgx-enclave-common-dev libsgx-uae-service
sudo usermod -aG sgx_prv $USER
sudo apt install -y libsgx-dcap-default-qpl libsgx-enclave-common-dev libsgx-dcap-ql-dev libsgx-dcap-default-qpl-dev
sudo apt install -y ocaml ocamlbuild automake autoconf libtool python-is-python3 git perl libcurl4-openssl-dev protobuf-compiler libprotobuf-dev debhelper reprepro unzip
git clone https://github.com/intel/linux-sgx.git
if [ ! -d "linux-sgx" ]; then
  echo "Error clone linux-sgx packages, please check github connection status, and run this scripts again"
  exit
else 
  cd linux-sgx && make preparation
  if [ "`ls -A external/toolset/ubuntu20.04/`" = "" ]; then
    echo "Error build linux-sgx packages, please check github connection status or local compiler environment , and run this scripts again"
    exit
  else
    sudo cp external/toolset/ubuntu20.04/* /usr/local/bin
  fi
fi

echo "Download SGX SSL"
if [ ! -f "lin_2.15.1_1.1.1l.zip" ]; then
  wget https://github.com/intel/intel-sgx-ssl/archive/refs/tags/lin_2.15.1_1.1.1l.zip
  if [ ! -f "lin_2.15.1_1.1.1l.zip" ]; then
    echo "Error download sgx ssl package from github, please check github connection status, and run this scripts again"
    exit
  else
    unzip lin_2.15.1_1.1.1l.zip
  fi
else
  unzip lin_2.15.1_1.1.1l.zip
fi

echo "Download OpenSSL for SGX SSL"
if [ ! -f "openssl-1.1.1l.tar.gz" ]; then
  wget https://www.openssl.org/source/old/1.1.1/openssl-1.1.1l.tar.gz
  cp openssl-1.1.1l.tar.gz intel-sgx-ssl-lin_2.15.1_1.1.1l/openssl_source/
else
  cp openssl-1.1.1l.tar.gz intel-sgx-ssl-lin_2.15.1_1.1.1l/openssl_source/
fi

echo "Install SGX SSL"
cd intel-sgx-ssl-lin_2.15.1_1.1.1l/Linux/
make all
make test
sudo make install

echo `ls /dev/ | grep -q "^sgx$"`
if [ $? -ne 0 ] ;then
    echo "Some problems may occur, please check whether the SGX function is turned on in the BIOS, restart the device, and run the script again"
else
    echo "Environment set done"
fi
