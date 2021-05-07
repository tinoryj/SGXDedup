#!/usr/bin/expect
# This is expect script for remote install, please replace ssh content to your local info.
for {set i 1} {$i <= 21} {incr i} {
    spawn ssh Testbed@node$i
    expect "yes/no" { 
    	send "yes\r"
    	expect "*?assword" { send "123456\r" }
    	} "*?assword" { send "123456\r" }
    expect "$*" { send "sudo apt update\r" }
    expect "$*" { send "sudo apt dist-upgrade\r" }
    expect "$*" { send "sudo apt autoremove\r" }
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
