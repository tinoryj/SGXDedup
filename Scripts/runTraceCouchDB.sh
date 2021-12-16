#!/bin/bash
prefix='/home/tinoryj/Trace/couchdbTrace/packed_enterprises/'
targetSnapSet=('2.5.2' '3.0.2' '3.0.3' '3.1.0' '3.1.3' '3.1.5' '3.1.6' '4.0.0' '4.1.0' '4.1.1' '4.5.0' '4.5.1' '4.6.0' '4.6.1' '4.6.2' '4.6.3' '4.6.4' '4.6.5' '5.0.1' '5.1.0' '5.1.1' '5.5.0' '5.5.1' '5.5.2' '6.0.0' '6.0.1' '6.0.2' '6.0.3' '6.0.4' '6.0.5' '6.5.0' '6.5.1' '6.6.0' '6.6.1' '6.6.2' 'community-2.2.0' 'community-3.0.1' 'community-3.1.3' 'community-4.0.0' 'community-4.1.0' 'community-4.1.1' 'community-4.5.0' 'community-4.5.1' 'community-5.0.1' 'community-5.1.1' 'community-6.0.0' 'community-6.5.0' 'community-6.5.1' 'community-6.6.0' 'community' 'enterprise-2.5.2' 'enterprise-3.0.2' 'enterprise-3.0.3' 'enterprise-3.1.0' 'enterprise-3.1.3' 'enterprise-3.1.5' 'enterprise-3.1.6' 'enterprise-4.0.0' 'enterprise-4.1.0' 'enterprise-4.1.1' 'enterprise-4.5.0' 'enterprise-4.5.1' 'enterprise-4.6.0' 'enterprise-4.6.1' 'enterprise-4.6.2' 'enterprise-4.6.3' 'enterprise-4.6.4' 'enterprise-4.6.5' 'enterprise-5.0.1' 'enterprise-5.1.0' 'enterprise-5.1.1' 'enterprise-5.5.0' 'enterprise-5.5.1' 'enterprise-5.5.2' 'enterprise-6.0.0' 'enterprise-6.0.1' 'enterprise-6.0.2' 'enterprise-6.0.3' 'enterprise-6.0.4' 'enterprise-6.0.5' 'enterprise-6.5.0' 'enterprise-6.5.1' 'enterprise')

./cleanRunTime.sh

for target in ${targetSnapSet[@]}; do
    echo "Upload $target"
    cp ${prefix}${target}/${target}.tar ./
    md5sum ${target}.tar
    sleep 3
    ./client-sgx -s ${target}.tar >>SGXDedup-CouchDB-upload.log
    rm -rf ${target}.tar
done

for target in ${targetSnapSet[@]}; do
    echo "Download $target"
    ./client-sgx -r ${target}.tar >>SGXDedup-CouchDB-download.log
    rm -rf ${target}.tar.d
done
