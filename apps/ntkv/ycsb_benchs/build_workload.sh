#!/bin/bash

payload=(
  "8"
  "64"
  "512"
  "1024"
  "2048"
  "4096"
)

for payload in ${payload[@]}; do
    cp ./workloads/workloada.spec ./test_workloads/R50-U50-payload$payload.spec 
    echo "fieldlength=$payload" >> ./test_workloads/R50-U50-payload$payload.spec 
done

for payload in ${payload[@]}; do
    cp ./workloads/workloadb.spec ./test_workloads/R90-U10-payload$payload.spec 
    echo "fieldlength=$payload" >> ./test_workloads/R90-U10-payload$payload.spec 
done

for payload in ${payload[@]}; do
    cp ./workloads/workloadc.spec ./test_workloads/R100-payload$payload.spec 
    echo "fieldlength=$payload" >> ./test_workloads/R100-payload$payload.spec 
done

for payload in ${payload[@]}; do
    cp ./workloads/workloadd.spec ./test_workloads/R95-I5-payload$payload.spec 
    echo "fieldlength=$payload" >> ./test_workloads/R95-I5-payload$payload.spec 
done

for payload in ${payload[@]}; do
    cp ./workloads/workloadf.spec ./test_workloads/R50-M50-payload$payload.spec 
    echo "fieldlength=$payload" >> ./test_workloads/R50-M50-payload$payload.spec 
done