for i in `seq 1 20`;
do
  time ./client ginny.socs.uoguelph.ca:12045 -f testFiles/wonderland.txt -b 400
done
