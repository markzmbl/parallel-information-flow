#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <cassert>

// A threadsafe-queue.
template <class T>
class SafeQueue
{
private:
  std::queue<T> q;
  mutable std::mutex queueMutex;
  mutable std::mutex* activationMutexes;
  std::condition_variable c;
  uint16_t size;
  mutable bool* activations;
  std::atomic<uint8_t> threadsWaiting;
  uint8_t threadsCount;
  bool finished;

  bool activate(T nodeId) {
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
    , activationMutexes()
    , c()
    , size(n)
    , threadsWaiting(0)
    , threadsCount(_threadsCount)
    , finished(false)
  {
    activations = new bool[size];
    activationMutexes = new std::mutex[size];
  }

  ~SafeQueue(void)
  {
    delete[] activations;
    delete[] activationMutexes;
  }

  // set activation sate

  // Add an element to the queue.
  bool enqueue(T nodeId)
  {
    bool activated = activate(nodeId);
    if (activated) {
      std::lock_guard<std::mutex> lock(queueMutex);
      q.push(nodeId);
      c.notify_one();
      return true;
    }
    return false;
  }

  // Get the "front"-element.
  // If the queue is empty, wait till a element is avaiable.
  std::shared_ptr<T> dequeue(void)
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    // enter waiting while loop
    // while the queue has no element and
    // not all threads are waiting and
    // there still exists one thread possible producing
    // queue items
    while(q.empty() && !finished)
    {
      // icrease waiting counter
      ++threadsWaiting;
      // check if final thread is going to wait
      if (threadsWaiting >= threadsCount) {
        // the simulation is finished
        finished = true;
        // notify all threads to end the wait
        c.notify_all();
        return nullptr;
      } else {
        // release lock as long as the wait and reaquire it afterwards.
        c.wait(lock);
      }
      --threadsWaiting;

    }
    if (q.empty())
      std::cout << 1;
    std::shared_ptr<T> valPtr = std::make_shared<T>(q.front());
    q.pop();
    return valPtr;
  }
  bool empty()
  {
    return q.empty();
  }
  void reset()
  {
    assert(q.empty());
    std::fill(activations, activations + size, false);
    threadsWaiting = 0;
    finished = false;
  }
  bool isActivated(T nodeId)
  {
    return activations[nodeId];
  }

};
#endif
