#!/bin/bash

# path of fsl trace
fsl='/home/testbed/Dataset/MS-UBC'

# users considered in backups
users=('1334' '1372' '1422' '1428' '1435' '1535' '1560' '1594' '1600' '1602')

for user in ${users[@]}; do
    echo ${user} 
	python transMS2FSL.py ${fsl}/${user}  ubctmp/${user}.ans
	./client-sgx -s ubctmp/${user}.ans  
	rm -rf ubctmp/${user}.ans
done

for user in ${users[@]}; do
    echo ${user} 
	./client-sgx -r ubctmp/${user}.ans  
	rm -rf ubctmp/${user}.ans.d
done
