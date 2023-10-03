#!/bin/bash

for i in `seq 1 $1`;do
    ./client 0.0.0.0 test2.c $2 $3 > "./outputs/out_$i.txt" &
done

wait

echo "All client processed"
total=0
for file in ./outputs/*.txt; do
    total=$(echo "$total + $(cat $file | grep "Average Response time" | awk -F":" '{print $2}')"|bc)
done 
total=$(echo "scale=5; $total/$1"|bc -l)
echo "$1 $total" >> "avgresponsevsM.txt"

rm -f ./outputs/out_*.txt
