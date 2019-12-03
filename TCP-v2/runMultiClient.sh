#!/bin/bash 

for i in `seq 1 10`; 
do 
    cp files/wonderland.txt files/wonderland$i.txt
done

for i in `seq 1 10`; 
do 
    ./client localhost:12045 -f files/wonderland$i.txt -b 10 &
done

./client localhost:12045 -f files/wonderland.txt -b 10 &





