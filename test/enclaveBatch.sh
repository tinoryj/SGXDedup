#!/bin/bash

# path of input file
file='/home/tinoryj/TestData/2G.file'

batchSizeSet=('1' '10' '100' '500' '1000' '2000'  '3000' '4000')

for batchSize in ${batchSizeSet[@]}; do
	cp ../test/config-${batchSize}.json ./config.json
	for i in {1..10};do
    	echo ${batchSize}-${i} 
		./client-sgx -s ${file}
		rm -rf .keyGenStore
	done
done

