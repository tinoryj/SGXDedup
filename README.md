# Accelerating Encrypted Deduplication via SGX

## Introduction

Encrypted deduplication preserves the deduplication effectiveness on encrypted data and is attractive for outsourced storage.  However, existing encrypted deduplication approaches build on expensive cryptographic primitives that incur substantial performance slowdown.  We present SGXDedup, which leverages Intel SGX to speed up encrypted deduplication based on server-aided message-locked encryption (MLE) while preserving security via SGX.  SGXDedup implements a suite of secure interfaces to execute MLE key generation and proof-of-ownership operations in SGX enclaves.  It also proposes various designs to support secure and efficient enclave operations.  Evaluation of synthetic and real-world workloads shows that SGXDedup achieves significant speedups and maintains high bandwidth and storage savings.

## Publication

## Prerequisites

We tested SGXDedup based on the Ubuntu 18.04.5 LTS version on a device with a Gigabyte B250M-D3H motherboard and an Intel i5-7400 CPU. 

Before everything starts, please check whether your device supports Intel SGX. Generally, the 7th generation and later Intel Core processors and their corresponding motherboards can enable the SGX function. Specifically, check whether there is an option marked as `SGX` or `Intel Software Guard Extensions` in your BIOS. If there is, adjust it to `Enable`, otherwise your device may not support the SGX function. We strongly recommend that you find a device that meets the requirements through [SGX-hardware list (Third-party on Github, click here)](https://github.com/ayeks/SGX-hardware).

After the device availability check is completed and the SGX function is successfully turned on in the BIOS, we begin preparations for SGXDedup compilation and execution.

### Registration in Intel for Remote Attestation Service

Since SGXDedup needs to use the `EPID based Remote Attestation` service provided by Intel, before starting, users need to register at [EPID-attestation page (click here)](https://api.portal.trustedservices.intel.com/EPID-attestation) to obtain the corresponding EPID and its corresponding subscription key.

After completes the registration, you can obtain the required EPID and subscription key through the [Products page (click here)](https://api.portal.trustedservices.intel.com/products) page. Here, our test uses the `DEV Intel® Software Guard Extensions Attestation Service (Unlinkable)` version of the service (users can also choose `Intel® Software Guard Extensions Attestation Service (Unlinkable)`). As shown in the figure below, the EPID and the corresponding subscription keys (Primary key and Secondary key) can be obtained, we will use this information in the configuration later.

![RA-Subscription](Docs/img/InkedRA-Subscription.jpg)

### Dependent Packages List

Here we give the download and installation method of each dependency. At the same time, in the `Packages/` directory of this project, the pre-downloaded version is provided. At the same time, in the `Docs/Guides/` directory, we give all the third-party documentation mentioned in this configuration guide.

#### Packages that need to be manually configured 

1. Intel® Software Guard Extensions (Intel® SGX) driver version 2.6.0_4f5bb53 [Download Link](https://download.01.org/intel-sgx/sgx-linux/2.7/distro/ubuntu18.04-server/sgx_linux_x64_driver_2.6.0_4f5bb63.bin)
2. Intel® SGX platform software (Intel® SGX PSW) version 2.7.100.4 [Download Link](https://download.01.org/intel-sgx/sgx-linux/2.7/distro/ubuntu18.04-server/libsgx-enclave-common_2.7.100.4-bionic1_amd64.deb)
3. Intel® SGX SDK version 2.7.100.4 [Download Link](https://download.01.org/intel-sgx/sgx-linux/2.7/distro/ubuntu18.04-server/sgx_linux_x64_sdk_2.7.100.4.bin)
4. Intel® SGX SSL version lin_2.5_1.1.1d [Download Link](https://github.com/intel/intel-sgx-ssl/archive/refs/tags/lin_2.5_1.1.1d.zip)
5. OpenSSL version 1.1.1d [Donwload Link](https://www.openssl.org/source/old/1.1.1/openssl-1.1.1d.tar.gz)
6. The cmake module used to compile the sgx program in the cmake system: `FindSGX.cmake` [Download Link](https://github.com/xzhangxa/SGX-CMake/blob/master/cmake/FindSGX.cmake)
#### Packages installed through package management tools such as `atp`

1. libssl-dev
2. libcurl4-openssl-dev
3. libprotobuf-dev
4. libboost-all-dev
5. libleveldb-dev
6. libsnappy-dev
7. build-essential
8. cmake
9. wget

#### Third-party Documents

1. [SGX Compact Hardwares List](Docs/Guides/SGXHardwares.md)
2. [Intel SGX Environment Install Guide](Docs/Guides/Intel_SGX_Installation_Guide_Linux_2.7_Open_Source.pdf)
3. [Intel SGX SSL Install Guide](Docs/Guides/Intel-SGX-SSL.md)

### Install Compilation Environment

SGXDedup is compiled and generated through the cmake system. We first install the compiler and other environments required for compilation and execution: 

```shell 
sudo apt install build-essential cmake wget libssl-dev libcurl4-openssl-dev libprotobuf-dev libboost-all-dev libleveldb-dev libsnappy-dev
```

Then we put the downloaded `FindSGX.cmake` into the modules directory under the cmake directory. In Ubuntu 18.04, the path should be `/usr/share/cmake-3.10/Modules`:  

```shell
cp FindSGX.cmake /usr/share/cmake-3.10/Modules/
```

### Install SGX Related Packages

In the current version (v1.0) of SGXDedup, we use the 2.7 version of the SGX SDK and the corresponding drivers and tools. These packages can be downloaded from [01.org (click here)](https://01.org/intel-softwareguard-extensions/downloads/intel-sgx-linux-2.7-release) 

Here, we download and install the following three software packages according to the [Official Installation Guide (click here)](https://download.01.org/intel-sgx/sgx-linux/2.7/docs/Intel_SGX_Installation_Guide_Linux_2.7_Open_Source.pdf) (page 5 to 10) provided by intel: 

1. Intel® Software Guard Extensions (Intel® SGX) driver [Download Link](https://download.01.org/intel-sgx/sgx-linux/2.7/distro/ubuntu18.04-server/sgx_linux_x64_driver_2.6.0_4f5bb63.bin)
2. Intel® SGX platform software (Intel® SGX PSW) [Download Link](https://download.01.org/intel-sgx/sgx-linux/2.7/distro/ubuntu18.04-server/libsgx-enclave-common_2.7.100.4-bionic1_amd64.deb)
3. Intel® SGX SDK [Download Link](https://download.01.org/intel-sgx/sgx-linux/2.7/distro/ubuntu18.04-server/sgx_linux_x64_sdk_2.7.100.4.bin)

After the three software packages are installed, we install the intel-sgx-ssl software package needed by SGXDedup according to the [official guide (click here to see)](https://github.com/intel/intel-sgx-ssl/tree/lin_2.5_1.1.1d). We use the [Linux 2.5 SGX SDK, OpenSSL 1.1.1d (click here to download)](https://github.com/intel/intel-sgx-ssl/archive/refs/tags/lin_2.5_1.1.1d.zip) version.

#### Example to Install SGX Environment with Ubuntu 18.04 LTS

For example, in ubuntu 18.04, we install the driver and PSW package according to the following commands. The packages will be installed into `/opt/intel` by default.

```shell
chmod +x sgx_linux_x64_driver_2.6.0_4f5bb63.bin
sudo ./sgx_linux_x64_driver_2.6.0_4f5bb63.bin
sudo dpkg -i ./libsgx-enclave-common_2.7.100.4-bionic1_amd64.deb
```

Finally, we install the SDK and configure environment variables by following commands.

```shell
chmod +x sgx_linux_x64_sdk_2.7.100.4.bin
sudo ./sgx_linux_x64_sdk_2.7.100.4.bin
source /opt/intel/sgxsdk/environment
```

Note that the default installation path of the SDK is the current path where the SDK package is located. The install program will alert you with 'Do you want to install in current directory? [yes/no]'. Here we enter `no` when selecting the installation path, and then give the installation path `/opt/intel` to make sure that the three software packages are installed in the same path.

#### Example to Install SGX SSL with Ubuntu 18.04 LTS

First, we follow the steps of the official tutorial to install. In the first step, we decompress the downloaded intel-sgx-ssl package:

```shell
unzip intel-sgx-ssl-lin_2.5_1.1.1d.zip
```

In the second step, we copy the source code compression package of OpenSSL to the `intel-sgx-ssl-lin_2.5_1.1.1d/openssl_source/`, and enter the `intel-sgx-ssl-lin_2.5_1.1.1d/Linux/`

```shell
cp openssl-1.1.1d.tar.gz intel-sgx-ssl-lin_2.5_1.1.1d/openssl_source/
cd intel-sgx-ssl-lin_2.5_1.1.1d/Linux/
```

The third step is to execute the compilation and installation instructions:

```shell
make all
make test
sudo make install
```

After the execution is complete, intel-sgx-ssl will be installed in the `/opt/intel/sgxssl/` directory. Here, because intel-sgx-ssl introduces a custom header file `pthread.h`, which will interfere with the normal compilation of SGXDedup, we use the following simple method to correct it:

```shell
cd /opt/intel/sgxssl/include
sudo mv pthread.h sgxpthread.h
sudo sed -i '415c #include \"sgxpthread.h\"' /opt/intel/sgxssl/include/openssl/crypto.h
```

These commands modify the imported `pthread.h` file name to avoid the compilation error of SGXDedup.

## Installation Guide

## Usage

### Uasge Samples