#!/bin/bash 

for i in `seq 1 4`; 
do 
    ./client localhost:12045 -f files/smallTest.txt -b 10 &
done

./client localhost:12045 -f files/smallTest.txt -b 10


