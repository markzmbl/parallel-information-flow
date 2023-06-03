#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>

// A threadsafe-queue.
template <class T>
class SafeQueue
{
private:
  std::queue<T> q;
  mutable std::mutex queueMutex;
  mutable std::mutex activationMutex;
  std::condition_variable c;
  uint16_t size;
  bool* activations;
  uint8_t threadsWaiting;
  uint8_t threadsCount;

  bool activate(T nodeId) {
    std::lock_guard<std::mutex> lock(activationMutex);
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
    , activationMutex()
    , c()
    , size(n)
    , threadsWaiting(0)
    , threadsCount(_threadsCount)
  {
    activations = new bool[size];
  }

  ~SafeQueue(void)
  {
    delete[] activations;
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
  T dequeue(void)
  {
    std::unique_lock<std::mutex> lock(queueMutex);
    while(q.empty())
    {
      // release lock as long as the wait and reaquire it afterwards.
      ++threadsWaiting;
      c.wait(lock);
      --threadsWaiting;
    }
    T val = q.front();
    q.pop();
    return val;
  }
  bool empty()
  {
    return q.empty();
  }
  void resetActivations()
  {
    activations = {0};
  }


};
#endif
