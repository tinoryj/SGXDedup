#!/bin/bash
prefix='/home/tinoryj/Trace/linuxTrace/'
targetSnapSet=('v2.6.11.tar' 'v2.6.12.tar' 'v2.6.13.tar' 'v2.6.14.tar' 'v2.6.15.tar' 'v2.6.16.tar' 'v2.6.17.tar' 'v2.6.18.tar' 'v2.6.19.tar' 'v2.6.20.tar' 'v2.6.21.tar' 'v2.6.22.tar' 'v2.6.23.tar' 'v2.6.24.tar' 'v2.6.25.tar' 'v2.6.26.tar' 'v2.6.27.tar' 'v2.6.28.tar' 'v2.6.29.tar' 'v2.6.30.tar' 'v2.6.31.tar' 'v2.6.32.tar' 'v2.6.33.tar' 'v2.6.34.tar' 'v2.6.35.tar' 'v2.6.36.tar' 'v2.6.37.tar' 'v2.6.38.tar' 'v2.6.39.tar' 'v3.0.tar' 'v3.1.tar' 'v3.2.tar' 'v3.3.tar' 'v3.4.tar' 'v3.5.tar' 'v3.6.tar' 'v3.7.tar' 'v3.8.tar' 'v3.9.tar' 'v3.10.tar' 'v3.11.tar' 'v3.12.tar' 'v3.13.tar' 'v3.14.tar' 'v3.15.tar' 'v3.16.tar' 'v3.17.tar' 'v3.18.tar' 'v3.19.tar' 'v4.0.tar' 'v4.1.tar' 'v4.2.tar' 'v4.3.tar' 'v4.4.tar' 'v4.5.tar' 'v4.6.tar' 'v4.7.tar' 'v4.8.tar' 'v4.9.tar' 'v4.10.tar' 'v4.11.tar' 'v4.12.tar' 'v4.13.tar' 'v4.14.tar' 'v4.15.tar' 'v4.16.tar' 'v4.17.tar' 'v4.18.tar' 'v4.19.tar' 'v4.20.tar' 'v5.0.tar' 'v5.1.tar' 'v5.2.tar' 'v5.3.tar' 'v5.4.tar' 'v5.5.tar' 'v5.6.tar' 'v5.7.tar' 'v5.8.tar' 'v5.9.tar' 'v5.10.tar' 'v5.11.tar' 'v5.12.tar' 'v5.13.tar')

./cleanRunTime.sh

for target in ${targetSnapSet[@]}; do
    echo "Upload $target"
    cp ${prefix}${target} ./
    md5sum ${target}
    sleep 3
    ./client-sgx -s ${target} >>SGXDedup-Linux-upload.log
    rm -rf ${target}
done

for target in ${targetSnapSet[@]}; do
    echo "Download $target"
    ./client-sgx -r ${target} >>SGXDedup-Linux-download.log
    rm -rf ${target}.d
done
