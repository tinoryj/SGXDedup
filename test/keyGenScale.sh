#!/bin/bash

keyGenNumberSet=('1' '5' '10' '15' '20' '25' '30' '35' '40')

for keyGenNumber in ${keyGenNumberSet[@]}; do
	for i in {1..10};do
    	echo ${keyGenNumber}-${i}-1
		./client-sgx -k ${keyGenNumber} 20480 4096
		sleep 5
		echo ${keyGenNumber}-${i}-2
		./client-sgx -k ${keyGenNumber} 20480 4096
		sleep 5
		rm -rf .keyGenStore*
	done
done

