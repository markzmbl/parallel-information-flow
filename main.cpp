#include <stdio.h>
#include <atomic>
#include <iostream>
#include <sstream>


#include "SafeQueue.hpp"

int main() {

    // count nodes
    uint16_t n = 1'718;
    // network adjacency matrix
    uint8_t* adjacencyMatrix = new uint8_t[n*n];
    // information access representation
    float* iar = new float[n*n];
    // probabilty to infect a node
    float alpha = 0.01f;
    // amount of simulations
    // uint16_t m = 10'000;
    uint16_t m = 100;
    // amount of threads
    uint8_t threadCount = 1;



    // Read the binary data
    FILE *file = fopen("adjacency_matrix.bin", "rb");
    size_t readBytes = fread(adjacencyMatrix, sizeof(uint8_t), n*n, file);
    fclose(file);

    SafeQueue<uint16_t> queue(n, threadCount);


    // iterate nodes
    for (uint16_t seedNodeId=0; seedNodeId<n; ++seedNodeId) {
        // iterate simulations
        for (uint16_t simulationId=0; simulationId<m; ++simulationId) {
            printf("nodeId:\t%u\tsimulationId:\t%u\n", seedNodeId, simulationId);

            // enqueue seed node
            queue.enqueue(seedNodeId);

            std::shared_ptr<uint16_t> activeNodeIdPtr;

            // enter simulation loop
            while (true) {

                // try to get next active node
                activeNodeIdPtr = queue.dequeue();

                // check if simulation is finished
                if (activeNodeIdPtr == nullptr) {
                    break;
                }

                uint16_t activeNodeId = *activeNodeIdPtr;

                // iterate its neighbors
                for (uint16_t nodeId=0; nodeId<n; ++nodeId) {
                    // check if edge exists from active node to other node
                    // check if coin flip infects
                    if (adjacencyMatrix[activeNodeId*n+nodeId] == 1 &&
                            (float)std::rand()/RAND_MAX <= alpha) {
                        
                        // True successfully enqueued
                        // node already has been activated
                        bool enqueued = queue.enqueue(nodeId);

                    }
                }
            }
            // accumulate activated nodes
            for (uint16_t nodeId=0; nodeId<n; ++nodeId) {
                if (queue.isActivated(nodeId)) {
                    iar[seedNodeId*n+nodeId] += 1.0f;
                }
            }
            // reset queue for next simulation
            queue.reset();
        }
    }
    // experiment finished
    delete[] adjacencyMatrix;
    // normalize information access representation
    // std::cout << "normalizing..." << std::flush;
    // for (uint16_t nodeId=0; nodeId<n*n; ++nodeId) {
    //     iar[nodeId] = iar[nodeId] / m;
    // }
    // std::cout << " finished" << std::endl;

    // write the binary data
    std::stringstream fileOutName;
    fileOutName << "threads_" << (size_t)threadCount << "_simulations_" << (size_t)m << ".bin";

    std::cout << "writing...";
    FILE *fileOut = fopen(
        fileOutName.str().c_str(), "wb"
    );
    size_t writtenBytes = fwrite(iar, sizeof(float), n*n, fileOut);
    fclose(fileOut);
        std::cout << " finished" << std::endl;



    delete[] iar;

    return 0;
}