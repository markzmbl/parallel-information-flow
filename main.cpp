#include <stdio.h>
#include <atomic>


#include "SafeQueue.hpp"

int main() {
    FILE *file = fopen("adjacency_matrix.bin", "rb");

    // Read the binary data
    uint16_t n = 1'718;
    uint8_t adjacencyMatrix[n*n];
    uint16_t counts[n*n];
    size_t readBytes = fread(adjacencyMatrix, sizeof(uint8_t), n*n, file);
    fclose(file);

    SafeQueue<uint16_t> queue;

    float alpha = 0.01f;
    // uint16_t m = 10'000;
    uint16_t m = 10;

    for (uint16_t seedNodeId=0; seedNodeId<n; ++seedNodeId) {
        // printf("nodeId:\t%u\n", seedNodeId);
        for (uint16_t simulationId=0; simulationId<m; ++simulationId) {
            printf("nodeId:\t%u\tsimulationId:\t%u\n", seedNodeId, simulationId);

            /*
            states:
            0: ready to recieve
            1: ready to transmit/dormant
            */
            uint8_t states[n] = {0};
            std::mutex stateMutexes[n];
            states[seedNodeId] = 1;
            queue.enqueue(seedNodeId);

            while (!queue.empty()) {
                // get next active node
                uint16_t activeNodeId = queue.dequeue();
                // iterate its neighbors
                for (uint16_t nodeId=0; nodeId<n; ++nodeId) {
                    // check if edge exists from active node to other node
                    // check if coin flip shall affect
                    if (adjacencyMatrix[activeNodeId*n+nodeId] == 1 &&
                            (float)std::rand()/RAND_MAX <= alpha) {
                        // lock operation on node state
                        stateMutexes[nodeId].lock();
                        // check if node is ready to recieve
                        if (states[nodeId] == 0) {
                            // node has successfully been infected
                            states[nodeId] = 1;
                            // and is appended to work queue
                            queue.enqueue(nodeId);
                        }
                        stateMutexes[nodeId].unlock();
                    }
                }
                // states[activeNodeId] = 2;
            }
            // accumulate activated nodes
            for (uint16_t nodeId=0; nodeId<n; ++nodeId) {
                counts[seedNodeId*n+nodeId] += states[nodeId];
            }
        }
        // // simulation finished
        // for (uint16_t nodeId=0; nodeId<n; ++nodeId) {
        //     iar[seedNodeId*n+nodeId] /= m;
        // }
    }

    return 0;
}