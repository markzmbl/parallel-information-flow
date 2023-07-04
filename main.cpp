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
#include "Graph.hpp"

void worker(
    Graph<uint32_t>& graph,
    SafeQueue<uint32_t>& queue,
    std::default_random_engine& rng
) {
    std::shared_ptr<uint32_t> activeNodeIdPtr;

    // enter simulation loop
    while (true) {

        // try to get next active node
        activeNodeIdPtr = queue.dequeue();

        // check if simulation is finished
        if (activeNodeIdPtr == nullptr) {
            break;
        }

        uint32_t activeNodeId = *activeNodeIdPtr;

        
        // iterate its neighbors        
        for (uint32_t nodeId : graph.getNeighbors(activeNodeId)) {
            // check if edge exists from active node to other node
            // check if coin flip infects
            float alpha = 1.0f / graph.getInDegree(nodeId);
            if (
                alpha == 1 ||
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
    Graph<uint32_t>& graph,
    SafeQueue<uint32_t>& queue,
    std::default_random_engine& rng,
    uint8_t threadsCount
) {
    if (threadsCount == 1) {
        worker(
            graph,
            queue, rng
        );
    } else {
        std::vector<std::thread> threads;
        // parallelized
        for (uint8_t i = 0; i < threadsCount; ++i) {
            threads.emplace_back(
                worker, std::ref(graph), std::ref(queue), std::ref(rng)
            );
        }

        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }
}

int main(int argc, char* argv[]) {


    Graph<uint32_t> graph;

    const std::string dataset = argv[1];
    // const std::string dataset = "amazon0601";
    // amount of simulations
    const uint32_t m = std::stoi(argv[2]);
    // const uint32_t m = 1000;
    // amount of threads
    // const uint8_t threadsCount = std::stoi(argv[3]);
    // const uint8_t threadsCount = 2;

    std::ifstream file(dataset+"/out."+dataset);

    std::string line;
    while (std::getline(file, line)) {

        if (line[0] != '%') {
            std::istringstream iss(line);

            uint32_t src, dest;
            iss >> src;
            iss.ignore();
            iss >> dest;

            graph.addEdge(src, dest);
        }
    }
    file.close();

    uint32_t n = graph.getVertexCount();

    for (uint8_t threadsCount=1; threadsCount<=6; ++threadsCount) {

        // information access representation
        // std::vector<float> iar(n * n);


        SafeQueue<uint32_t> queue(n, threadsCount);

        // Create a random number generator
        std::random_device rd;
        std::default_random_engine rng(rd());
    


        // iterate nodes
        for (uint32_t seedNodeId = 0; seedNodeId < n; ++seedNodeId) {
            // std::cout << "nodeId:\t" << seedNodeId << std::endl;

            // iterate simulations
            for (uint32_t simulationId = 0; simulationId < m; ++simulationId) {
                // std::cout << "nodeId:\t" << seedNodeId << "\tsimulationId:\t" << simulationId << std::endl;
                // enqueue seed node


                queue.enqueue(seedNodeId);
                auto start = std::chrono::high_resolution_clock::now();

                benchmark(
                    graph,
                    queue, rng,
                    threadsCount
                );

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> duration = end - start;

                std::cout
                    << (size_t) threadsCount << ","
                    << (size_t) seedNodeId << ","
                    << (size_t) simulationId << ","
                    << duration.count() << std::endl;
                queue.reset();

                // // accumulate activated nodes
                // #pragma omp parallel for simd
                // for (uint32_t nodeId = 0; nodeId < n; ++nodeId) {
                //     bool activated = queue.isActivated(nodeId);
                //     iar[seedNodeId * n + nodeId] += static_cast<float>(activated);
                // }
                // reset queue for next simulation
            }
        }
    }
    // experiment finished

    // normalize information access representation
    // std::cout << "normalizing..." << std::flush;
    // float factor = 1.0f / m;
    // #pragma omp parallel for simd
    // for (uint32_t nodeId = 0; nodeId < n * n; ++nodeId) {
    //     iar[nodeId] *= factor;
    // }

    // write the binary data
    // std::stringstream fileOutName;
    // fileOutName << "threads_" << static_cast<size_t>(threadsCount) << "_simulations_" << static_cast<size_t>(m) << ".bin";

    // std::ofstream fileOut(fileOutName.str(), std::ios::binary);
    // fileOut.write(reinterpret_cast<const char*>(iar.data()), iar.size() * sizeof(float));
    // fileOut.close();
    /*
    // count nodes
    uint32_t n = 1'718;
    // network adjacency matrix
    std::vector<uint8_t> adjacencyMatrix(n * n);
    // probabilty to infect a node
    float alpha = 0.01f;

    // mode
    const std::string mode = argv[1];
    // const std::string mode = "ira";


    // Read the binary data
    std::ifstream file("adjacency_matrix.bin", std::ios::binary);
    file.read(reinterpret_cast<char*>(adjacencyMatrix.data()), adjacencyMatrix.size() * sizeof(uint8_t));
    file.close();



    if (mode == "sampling") {
        // const uint8_t sampleSize = std::stoi(argv[4]);
        const uint8_t sampleSize = std::stoi(argv[4]);
        // iterate simulations
        for (uint32_t simulationId = 0; simulationId < m; ++simulationId) {
            // std::cout << "nodeId:\t" << seedNodeId << "\tsimulationId:\t" << simulationId << std::endl;
            // enqueue seed node
            std::vector<uint32_t> seeds(n);
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
    }
    */
    return 0;
}
