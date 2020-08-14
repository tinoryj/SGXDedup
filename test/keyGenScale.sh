#!/bin/bash

keyGenNumberSet=('1' '2' '4' '8' '16' '32' '64')

for keyGenNumber in ${keyGenNumberSet[@]}; do
	for i in {1..15};do
    	echo ${keyGenNumber}-${i} 
		./client-sgx -k ${keyGenNumber} 21000
		sleep 10
		./client-sgx -k ${keyGenNumber} 21000
		sleep 10
		rm -rf .keyGenStore*
	done
done

