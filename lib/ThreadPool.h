// This class is inspired by: https://github.com/progschj/ThreadPool
// I added additional functionaly for waiting for all threads to finish their task
// in a batch-wise fashion.

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

class ThreadPool
{
  public:
    ThreadPool(size_t);
    ~ThreadPool();
    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>;

    void waitAll();
    void barrier(); // Custom barrier function

  private:
    // synchronization
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable all_done;
    std::condition_variable barrier_condition;
    std::atomic<size_t> tasks_in_progress;
    std::atomic<int> barrier_count; // Counter for barrier
    std::atomic<int> tasks_count = 0;
    size_t total_workers;
    bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
    : stop(false), tasks_in_progress(0), total_workers(threads), barrier_count(0)
{
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back([this] {
            for (;;)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty())
                        return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                task();
            }
        });
}

inline void ThreadPool::barrier()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    // Increment the barrier count as threads reach the barrier
    int local_count = ++barrier_count;

    // Check if this is the last thread to reach the barrier
    if (local_count == total_workers) {
        // Reset the barrier count to zero for future use
        barrier_count = 0; 
        // Notify all waiting threads
        barrier_condition.notify_all();
    } else {
        // Wait until barrier_count is reset, which signifies all threads have reached the barrier
        barrier_condition.wait(lock, [this, local_count] { return barrier_count == 0; });
    }
}

inline void ThreadPool::waitAll()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    all_done.wait(lock, [this]() { return this->tasks.empty() && this->tasks_in_progress == 0; });
}

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task =
        std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::lock_guard<std::mutex> lock(queue_mutex);

        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task, this]() {
            (*task)();
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                tasks_count--;
                if (tasks_count == 0)
                {
                    all_done.notify_all();
                }
            }
        });

        tasks_count++;
    }

    condition.notify_one();
    return res;
}
// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
        worker.join();
}

#endif
