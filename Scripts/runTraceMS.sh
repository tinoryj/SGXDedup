#!/bin/bash
prefix='/home/tinoryj/Trace/MS'
snapshots=('0478' '0880' '1607' '1757' '2521' '2622' '3303' '4097' '5154' '5253')

./cleanRunTime.sh
rm -rf tmp
mkdir tmp
for snapshot in ${snapshots[@]}; do
        echo "Upload ${snapshot}"
        python3 transMS2FSL.py ${prefix}/${snapshot} ./tmp/${snapshot}.chunk >>msTraceInfo.log
        ./client-sgx -s tmp/${snapshot}.chunk >>SGXDedup-MS-upload.log
        rm -rf tmp/*
done

for snapshot in ${snapshots[@]}; do
        echo "Download ${snapshot}"
        ./client-sgx -r tmp/${snapshot}.chunk >>SGXDedup-MS-download.log
        rm -rf tmp/*
done
