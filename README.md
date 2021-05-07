# Accelerating Encrypted Deduplication via SGX

## Introduction

Encrypted deduplication preserves the deduplication effectiveness on encrypted data and is attractive for outsourced storage.  However, existing encrypted deduplication approaches build on expensive cryptographic primitives that incur substantial performance slowdown.  We present SGXDedup, which leverages Intel SGX to speed up encrypted deduplication based on server-aided message-locked encryption (MLE), while preserving security via SGX.  SGXDedup implements a suite of secure interfaces to execute MLE key generation and proof-of-ownership operations in SGX enclaves.  It also proposes various designs to support secure and efficient enclave operations.  Evaluation on synthetic and real-world workloads shows that SGXDedup achieves significant speedups and maintains high bandwidth and storage savings.

## Publication

## Prerequisites

We test SGXDedup under Ubuntu 18.04.5 LTS. They require the following third-party packages:
* OpenSSL (version 1.1.1d), 
* Leveldb (version 1.22), 
* Boost C++ library (version 1.65.1)
* Snappy (version 1.1.7-1)
* SGX Driver (version 2.6.0_4f5bb63)
* SGX SDK (version 2.7.101.3)
*  
*  Our test compiles TED and TEDStore using CMake 3.17 and GNU GCC 7.4.0.

    expect "$*" { send "sudo apt install openssl gcc gdb build-essential cmake clang llvm libboost-all-dev\r" }
    expect "$*" { send "cd ./dedupsgx/\r" }
    expect "$*" { send "chmod +x sgx_linux_x64_driver_2.6.0_4f5bb63.bin\r" }
    expect "$*" { send "sudo ./sgx_linux_x64_driver_2.6.0_4f5bb63.bin\r" }
    expect "$*" { send "sudo dpkg -i libsgx-enclave-common_2.7.101.3-xenial1_amd64.deb\r" }
    expect "$*" { send "chmod +x sgx_linux_x64_sdk_2.7.101.3.bin\r" }
    expect "$*" { send "sudo ./sgx_linux_x64_sdk_2.7.101.3.bin\r" }
    expect "yes/no" { send "no\r" }  
    expect "install in" { send "/opt/intel\r"}
    expect "/opt/intel/sgxsdk/environment" {
        send "source /opt/intel/sgxsdk/environment\r"
    }
    expect "$*" { send "cd intel-sgx-ssl-master\r" }
    expect "$*" { send "cd Linux\r" }
    expect "$*" { send "make all\r" }
    expect "$*" { send "make test\r" }
    expect "$*" { send "sudo make install\r" }
    expect "$*" { send "cd /opt/intel/sgxssl/include\r" }
    expect "$*" { send "sudo mv pthread.h sgxpthread.h\r" }
    expect "$*" { send "sudo sed -i '415c #include \"sgxpthread\"' /opt/intel/sgxssl/include/openssl/crypto.h \r" }
    expect "$*" { send "cat /opt/intel/sgxssl/include/openssl/crypto.h \r" }
    puts "\n node $i done"
}


## Installation Guide

## Usage

### Uasge Samples