#!/bin/bash


echo "threadsCount,nodeId,simulationId,duration"
# for i in {1..6}; do
for i in {1..6}; do
    ./main iar $1 $i
done
