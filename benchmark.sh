#!/bin/bash


# Iterate from 1 to 6
echo "threadsCount,nodeId,simulationId,duration"
for i in {1..6}; do
    # Call the function with the iteration variable
    ./main $1 $i
done
