#!/bin/bash
# ./vmstat_log.sh $1 &
# vmstat_pid=$!
# echo $vmstat_pid
# ./ps_log.sh $1 &
if [ $# -ne 5 ];then
    echo "usage: ./loadtest.sh <#clients> <#loops> <sleeptime> <Timeout>"
    exit 1
fi
for i in `seq 1 $1`;do
    echo $i
    ./client 0.0.0.0:$5 test2.c $2 $3 $4 > "./outputs/out_$i.txt" &
done

wait
# kill $vmstat_pid
echo "All client processed"
total=0
total_throughput=0
for file in ./outputs/*.txt; do
    # echo $file
    total=$(echo "$total + $(cat $file | grep "Average Response time" | awk -F":" '{print $2}') * $2"|bc)
    total_throughput=$(echo "$total_throughput + $(cat $file | grep "Throughput" | awk -F":" '{print $2}')"|bc)
    goodput=$(echo "$total_throughput + $(cat $file | grep "Goodput" | awk -F":" '{print $2}')"|bc)
    # echo $total $total_throughput $goodput
done 
totaltimetaken=$(echo "scale=5; ($total*$2)"|bc -l)
totalRequest=$(echo "($1*$2)"|bc -l)
requestRate=$(echo "scale=5; $totalRequest/$totaltimetaken"|bc -l)
total=$(echo "scale=5; $total/($1*$2)"|bc -l)
echo "$1 $total $total_throughput $goodput $requestRate" >> "avgresponsevsM.txt"


rm -f ./outputs/out_*.txt
