#!/bin/bash

batchSizeSet=('1' '2' '4' '8' '16' '32'  '64' '128' '256' '512' '1024' '2048' '4096' '8192' '16384' '32768')

for batchSize in ${batchSizeSet[@]}; do
	cp ../test/config-${batchSize}.json ./config.json
	for i in {1..3};do
    	echo ${batchSize}-${i} 
		./client-sgx -s 2G.file
		sleep 5
		./client-sgx -s 2G.file
		sleep 5
		rm -rf .keyGenStore*
	done
done

