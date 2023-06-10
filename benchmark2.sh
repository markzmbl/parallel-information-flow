#!/bin/bash


# Iterate from 1 to 6
echo "threadsCount,sampleSize,simulationId,duration"
# for i in {1..6}; do
for i in {1..2}; do
    for j in {2..30}; do
        # Call the function with the iteration variable
        ./main sampling $1 $i $j
    done
done
