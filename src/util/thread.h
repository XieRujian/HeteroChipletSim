#ifndef THREAD_H
#define THREAD_H

#include <atomic>
#include <queue>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <future>
#include <optional>

namespace util
{
    class TaskManager
    {
        class TaskQueue
        {
            mutable std::mutex mtx_;
            std::queue<std::function<void()>> queue_;
            std::condition_variable cv_;
            bool stop_ = false;

        public:
            TaskQueue() = default;
            TaskQueue(const TaskQueue &) = delete;
            TaskQueue &operator=(const TaskQueue &) = delete;
            ~TaskQueue() { stop(); }

            void push(std::function<void()> &&value)
            {
                {
                    std::lock_guard<std::mutex> lock(mtx_);
                    queue_.push(std::move(value));
                }
                cv_.notify_one();
            }

            bool pop(std::function<void()> &value)
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [this]
                         { return !queue_.empty() || stop_; });
                if (stop_ && queue_.empty())
                    return false;
                value = std::move(queue_.front());
                queue_.pop();
                return true;
            }

            void stop()
            {
                {
                    std::lock_guard<std::mutex> lock(mtx_);
                    stop_ = true;
                }
                cv_.notify_all();
            }
        } task_queue_;
        std::vector<std::thread> workers_;

    public:
        TaskManager(size_t thread_num = std::thread::hardware_concurrency())
        {
            for (size_t i = 0; i < thread_num; i++)
                workers_.emplace_back(
                    [this]()
                    {
                        std::function<void()> task;
                        while (task_queue_.pop(task))
                            task();
                    });
        }
        ~TaskManager()
        {
            task_queue_.stop();
            for (auto &worker : workers_)
                worker.join();
        }
        void PushTask(std::function<void()> &&task) { task_queue_.push(std::move(task)); }

        template <typename T>
        void RunTasks(const std::function<void(T &)> &func, std::vector<T> &args)
        {
            std::atomic<size_t> remaining(args.size());
            std::promise<void> p;
            std::future<void> f = p.get_future();

            for (size_t i = 0; i < args.size(); i++)
            {
                T *arg_ptr = &args[i];
                PushTask(
                    [this, &func, arg_ptr, &remaining, &p]()
                    {
                        func(*arg_ptr);
                        if (!--remaining)
                            p.set_value();
                    });
            }
            f.get();
        }

        template <typename T>
        void RunTasks(const std::function<void(const T &)> &func, const std::vector<T> &args)
        {
            std::atomic<size_t> remaining(args.size());
            std::promise<void> p;
            std::future<void> f = p.get_future();

            for (size_t i = 0; i < args.size(); i++)
            {
                const T* arg_ptr = &args[i];
                PushTask(
                    [this, &func, arg_ptr, &remaining, &p]()
                    {
                        func(*arg_ptr);
                        if (!--remaining)
                            p.set_value();
                    });
            }
            f.get();
        }
    } inline task_manager;
}

#endif /* THREAD_H */
