#include <string>
#include "lib/httplib.h"

#include <filesystem>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>

// Task structure
struct Task {
  std::string id;
  std::string data;
  bool isShutdownCommand = false;
};

/********************************************************
 *      setting up data structure for a task queue
 ********************************************************/
// Thread-safe queue for tasks
class MyTaskQueue {
 public:
  void push(Task task) {  // this will add a single task at a time.
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(task);
  }

  bool pop(Task& task) {  // this should pop one task at a time.
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
      return false;
    }
    task = m_queue.front();
    m_queue.pop();
    return true;
  }

  // New function to print all tasks
  void printAndRestore() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<Task> temp = m_queue;  // Make a copy of the queue

    std::cout << "Current Queue: ";
    while (!temp.empty()) {
      Task task = temp.front();
      std::cout << task.data << " ";
      temp.pop();
    }
    std::cout << std::endl;
  }

  std::queue<Task> m_queue;
 private:
  
  std::mutex m_mutex;
};
