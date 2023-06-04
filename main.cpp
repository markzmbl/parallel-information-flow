#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <atomic>
#include <sstream>
#include <omp.h>

#include "SafeQueue.hpp"

void worker(
    std::vector<uint8_t>& adjacencyMatrix,
    uint16_t n,
    float alpha,
    SafeQueue<uint16_t>& queue
) {
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
        for (uint16_t nodeId = 0; nodeId < n; ++nodeId) {
            // check if edge exists from active node to other node
            // check if coin flip infects
            if (adjacencyMatrix[activeNodeId * n + nodeId] == 1 &&
                (float)std::rand() / RAND_MAX <= alpha) {

                // True successfully enqueued
                // node already has been activated
                bool enqueued = queue.enqueue(nodeId);
            }
        }
    }
}

int main() {

    // count nodes
    uint16_t n = 1'718;
    // network adjacency matrix
    std::vector<uint8_t> adjacencyMatrix(n * n);
    // information access representation
    std::vector<float> iar(n * n);
    // probabilty to infect a node
    float alpha = 0.01f;
    // amount of simulations
    // uint16_t m = 10'000;
    uint16_t m = 2;
    // amount of threads
    uint8_t threadCount = 1;

    // Read the binary data
    std::ifstream file("adjacency_matrix.bin", std::ios::binary);
    file.read(reinterpret_cast<char*>(adjacencyMatrix.data()), adjacencyMatrix.size() * sizeof(uint8_t));
    file.close();

    SafeQueue<uint16_t> queue(n, threadCount);

    // iterate nodes
    for (uint16_t seedNodeId = 0; seedNodeId < n; ++seedNodeId) {
        // iterate simulations
        for (uint16_t simulationId = 0; simulationId < m; ++simulationId) {
            std::cout << "nodeId:\t" << seedNodeId << "\tsimulationId:\t" << simulationId << std::endl;

            // enqueue seed node
            queue.enqueue(seedNodeId);

            // parallelized
            worker(adjacencyMatrix, n, alpha, queue);
            // accumulate activated nodes
            #pragma omp parallel for simd
            for (uint16_t nodeId = 0; nodeId < n; ++nodeId) {
                bool activated = queue.isActivated(nodeId);
                iar[seedNodeId * n + nodeId] += static_cast<float>(activated);
            }
            // reset queue for next simulation
            queue.reset();
        }
    }
    // experiment finished

    // normalize information access representation
    std::cout << "normalizing..." << std::flush;
    float factor = 1.0f / m; 
    #pragma omp parallel for simd
    for (uint16_t nodeId = 0; nodeId < n * n; ++nodeId) {
        iar[nodeId] *= factor;
    }
    std::cout << " finished" << std::endl;

    // write the binary data
    std::stringstream fileOutName;
    fileOutName << "threads_" << (size_t)threadCount << "_simulations_" << (size_t)m << ".bin";

    std::cout << "writing...";
    std::ofstream fileOut(fileOutName.str(), std::ios::binary);
    fileOut.write(reinterpret_cast<const char*>(iar.data()), iar.size() * sizeof(float));
    fileOut.close();
    std::cout << " finished" << std::endl;

    return 0;
}
