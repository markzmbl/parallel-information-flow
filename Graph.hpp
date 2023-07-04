#ifndef GRAPH
#define GRAPH

#include <iostream>
#include <vector>
#include <unordered_set>


template <class T>
class Graph {
private:
    std::vector<std::vector<T>> adjacencyList;
    std::vector<T> inDegrees;
    T vertexCount;

public:
    Graph() : vertexCount(0) {}

    void addEdge(T src, T dest) {
        ensureVertexExists(src);
        ensureVertexExists(dest);

        if (!hasEdge(src, dest)) {
            adjacencyList[src].push_back(dest);
            inDegrees[dest]++;
        }

    }

    void addVertex() {
        adjacencyList.emplace_back();
        inDegrees.emplace_back(0);
        vertexCount++;
    }

    void ensureVertexExists(T vertex) {
        while (vertex >= vertexCount) {
            addVertex();
        }
    }

    T getVertexCount() {
        return vertexCount;
    }

    T getInDegree(T nodeId) {
        return inDegrees[nodeId];
    }

    bool hasEdge(T src, T dest) const {
        if (src < 0 || src >= vertexCount || dest < 0 || dest >= vertexCount)
            return false;

        const std::vector<T>& neighbors = getNeighbors(src);
        return std::find(neighbors.begin(), neighbors.end(), dest) != neighbors.end();
    }

    const std::vector<T>& getNeighbors(T nodeId) const {
        return adjacencyList[nodeId];
    }
};


#endif
