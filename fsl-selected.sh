#!/bin/bash

# path of fsl trace
fsl='/home/testbed/Dataset/FSL/2013'

# users considered in backups
# users=('004' '007' '008' '012' '013' '015' '022' '026' '028')
users=('012' '015')
# auxiliary information
date_of_aux=('2013-01-22' '2013-02-22' '2013-03-22' '2013-04-22' '2013-05-17')

hasher_outputs='tmp'
# count prior backups (as auxiliary information) and launch frequency analysis
for date in ${date_of_aux[@]}; do
    for user in ${users[@]}; do
	    snapshot="fslhomes-user${user}-${date}"
	    if [ -f "${fsl}"/${snapshot}.tar.gz ]; then
            echo "Find ID: ${user}-${date}:"
		    tar zxf "${fsl}"/${snapshot}.tar.gz  
		    fs-hasher/hf-stat -h ${snapshot}/${snapshot}.8kb.hash.anon > tmp/${snapshot} 
            echo "ID: ${user}-${date}:" 
		    ./client-sgx -s ./tmp/${snapshot}
		    rm -rf ${snapshot}
		    rm -rf ./tmp/${snapshot}
	    fi
    done
done

for date in ${date_of_aux[@]}; do
    for user in ${users[@]}; do
	    snapshot="fslhomes-user${user}-2013-${date}"
        echo "ID: ${user}-${date}:" 
		./client-sgx -r ./tmp/${snapshot}
		rm -rf ./tmp/${snapshot}.d
    done
done