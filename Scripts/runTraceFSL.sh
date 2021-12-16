#!/bin/bash
prefix='/home/tinoryj/Trace/FSL'
users=('007')
date_of_aux=('2013-01-22' '2013-01-29' '2013-02-05' '2013-02-12' '2013-02-19' '2013-03-16' '2013-03-23' '2013-03-30' '2013-04-06' '2013-04-13')

./cleanRunTime.sh
rm -rf tmp
mkdir tmp
# count prior backups (as auxiliary information) and launch frequency analysis
for date in ${date_of_aux[@]}; do
    for user in ${users[@]}; do
        snapshot="fslhomes-user${user}-${date}"
        if [ -f "${prefix}"/${snapshot}.tar.gz ]; then
            echo "Find ID: ${user}-${date}:"
            tar zxf "${prefix}"/${snapshot}.tar.gz
            fs-hasher/hf-stat -h ${snapshot}/${snapshot}.8kb.hash.anon >tmp/${snapshot}
            echo "Upload ID: ${user}-${date}:"
            ./client-sgx -s ./tmp/${snapshot} >>SGXDedup-FSL-upload.log
            rm -rf ${snapshot}
            rm -rf ./tmp/${snapshot}
        fi
    done
done

for date in ${date_of_aux[@]}; do
    for user in ${users[@]}; do
        snapshot="fslhomes-user${user}-${date}"
        echo "Downlaod ID: ${user}-${date}:"
        ./client-sgx -r ./tmp/${snapshot} >>SGXDedup-FSL-download.log
        rm -rf ./tmp/${snapshot}.d
    done
done
