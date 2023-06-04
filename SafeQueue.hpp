#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <cassert>
#include <vector>

// A threadsafe-queue.
template <class T>
class SafeQueue
{
private:
    std::queue<T> q;
    mutable std::mutex queueMutex;
    std::vector<std::mutex> activationMutexes;
    std::condition_variable c;
    uint16_t size;
    std::vector<bool> activations;
    std::atomic<uint8_t> threadsWaiting;
    uint8_t threadsCount;
    bool finished;

    bool activate(T nodeId)
    {
        std::lock_guard<std::mutex> lock(activationMutexes[nodeId]);
        if (!activations[nodeId]) {
            activations[nodeId] = true;
            return true;
        }
        return false;
    }

public:
    SafeQueue(T n, uint8_t _threadsCount)
        : q()
        , queueMutex()
        , activationMutexes(n)
        , c()
        , size(n)
        , activations(n, false)
        , threadsWaiting(0)
        , threadsCount(_threadsCount)
        , finished(false)
    {
    }

    ~SafeQueue(void) = default;

    // Add an element to the queue.
    bool enqueue(T nodeId)
    {
        bool activated = activate(nodeId);
        if (activated) {
            // std::lock_guard<std::mutex> lock(queueMutex);
            q.push(nodeId);
            c.notify_one();
            return true;
        }
        return false;
    }

    // Get the "front"-element.
    // If the queue is empty, wait till an element is available.
    std::shared_ptr<T> dequeue(void)
    {
		std::unique_lock<std::mutex> lock(queueMutex);
		// enter waiting while loop
		// while the queue has no element and
		// not all threads are waiting and
		// there still exists one thread possibly producing
		// queue items
		while (q.empty() && !finished)
		{
			// Increase waiting counter
			++threadsWaiting;

			if (threadsWaiting >= threadsCount) {
				finished = true;

				c.notify_all();
				break;
			}

			// Spurious wake-up safe wait
			c.wait(lock, [this] { return !q.empty() || finished; });

			--threadsWaiting;
		}

		if (!q.empty()) {
			std::shared_ptr<T> valPtr = std::make_shared<T>(q.front());
			// std::cout << "...bam..." << std::flush;
			q.pop();
			return valPtr;
		}

		std::shared_ptr<T> nullPtr(nullptr);
		return nullPtr;
	}

    bool empty()
    {
        return q.empty();
    }

    void reset()
    {
        assert(q.empty());
        std::fill(activations.begin(), activations.end(), false);
        threadsWaiting = 0;
        finished = false;
    }

    bool isActivated(T nodeId)
    {
        return activations[nodeId];
    }
};

#endif
