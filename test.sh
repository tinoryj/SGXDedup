#!/bin/bash

for ((i=0;i<=100;i=i+5));
do 
    for j in {1..10}
    do
        sleep 5
        echo "$i-$j test: ">>result-hash
        ./client -k $i 1000000 >> result-hash
    done
done