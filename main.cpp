#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <atomic>
#include <sstream>
#include <thread>
#include <omp.h>
#include <algorithm>

#include "SafeQueue.hpp"

void worker(
    std::vector<uint8_t>& adjacencyMatrix,
    uint16_t n,
    float alpha,
    SafeQueue<uint16_t>& queue,
    std::default_random_engine& rng
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
                std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) <= alpha
            ) {

                // True successfully enqueued
                // node already has been activated
                bool enqueued = queue.enqueue(nodeId);
            }
        }
    }
}

void benchmark(
    std::vector<uint8_t>& adjacencyMatrix,
    uint16_t n,
    float alpha,
    SafeQueue<uint16_t>& queue,
    std::default_random_engine& rng,
    uint8_t threadsCount
) {
    if (threadsCount == 1) {
        worker(
            adjacencyMatrix,
            n, alpha, queue, rng
        );
    } else {
        std::vector<std::thread> threads;
        // parallelized
        for (uint8_t i = 0; i < threadsCount; ++i) {
            threads.emplace_back(
                worker, std::ref(adjacencyMatrix), n, alpha, std::ref(queue), std::ref(rng)
            );
        }

        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }
}

int main(int argc, char* argv[]) {
    // count nodes
    uint16_t n = 1'718;
    // network adjacency matrix
    std::vector<uint8_t> adjacencyMatrix(n * n);
    // information access representation
    std::vector<float> iar(n * n);
    // probabilty to infect a node
    float alpha = 0.01f;

    // mode
    const std::string mode = argv[1];
    // const std::string mode = "ira";
    // amount of simulations
    const uint16_t m = std::stoi(argv[2]);
    // const uint16_t m = 10;
    // amount of threads
    const uint8_t threadsCount = std::stoi(argv[3]);
    // const uint8_t threadsCount = 2;


    // Read the binary data
    std::ifstream file("adjacency_matrix.bin", std::ios::binary);
    file.read(reinterpret_cast<char*>(adjacencyMatrix.data()), adjacencyMatrix.size() * sizeof(uint8_t));
    file.close();

    SafeQueue<uint16_t> queue(n, threadsCount);

    // Create a random number generator
    std::random_device rd;
    std::default_random_engine rng(rd());

    if (mode == "sampling") {
        // const uint8_t sampleSize = std::stoi(argv[4]);
        const uint8_t sampleSize = std::stoi(argv[4]);
        // iterate simulations
        for (uint16_t simulationId = 0; simulationId < m; ++simulationId) {
            // std::cout << "nodeId:\t" << seedNodeId << "\tsimulationId:\t" << simulationId << std::endl;
            // enqueue seed node
            std::vector<uint16_t> seeds(n);
            std::iota(seeds.begin(), seeds.end(), 0);
            std::shuffle(seeds.begin(), seeds.end(), rng);

            seeds.resize(sampleSize);

            for (uint8_t i=0; i < seeds.size(); ++i) {
                queue.enqueue(seeds[i]);
            }

            auto start = std::chrono::high_resolution_clock::now();

            benchmark(
                adjacencyMatrix,
                n, alpha, queue, rng,
                threadsCount
            );

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end - start;

            std::cout
                << (size_t) threadsCount << ","
                << (size_t) sampleSize << ","
                << (size_t) simulationId << ","
                << duration.count() << std::endl;

            // reset queue for next simulation
            queue.reset();
        }

    } else {
        // iterate nodes
        for (uint16_t seedNodeId = 0; seedNodeId < n; ++seedNodeId) {
            // std::cout << "nodeId:\t" << seedNodeId << std::endl;

            // iterate simulations
            for (uint16_t simulationId = 0; simulationId < m; ++simulationId) {
                // std::cout << "nodeId:\t" << seedNodeId << "\tsimulationId:\t" << simulationId << std::endl;
                // enqueue seed node
                queue.enqueue(seedNodeId);

                auto start = std::chrono::high_resolution_clock::now();

                benchmark(
                    adjacencyMatrix,
                    n, alpha, queue, rng,
                    threadsCount
                );

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> duration = end - start;

                std::cout
                    << (size_t) threadsCount << ","
                    << (size_t) seedNodeId << ","
                    << (size_t) simulationId << ","
                    << duration.count() << std::endl;

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
        // std::cout << "normalizing..." << std::flush;
        float factor = 1.0f / m;
        #pragma omp parallel for simd
        for (uint16_t nodeId = 0; nodeId < n * n; ++nodeId) {
            iar[nodeId] *= factor;
        }

        // write the binary data
        std::stringstream fileOutName;
        fileOutName << "threads_" << static_cast<size_t>(threadsCount) << "_simulations_" << static_cast<size_t>(m) << ".bin";

        std::ofstream fileOut(fileOutName.str(), std::ios::binary);
        fileOut.write(reinterpret_cast<const char*>(iar.data()), iar.size() * sizeof(float));
        fileOut.close();
    }

    return 0;
}
