#include "ThreadPool.h"

ThreadPool::ThreadPool(int size) : stop(false) {
    for (int i = 0; i < size; i++) {
        threads.emplace_back(std::thread([this]() { //  启动size个线程
            while (true) { 
                std::function<void()> task;
                { //在这个{}作用域内对std::mutex加锁，出了作用域会自动解锁，不需要调用unlock()
                    std::unique_lock<std::mutex> lock(tasks_mtx);
                    cv.wait(lock, [this](){ //等待条件变量，条件为任务队列不为空或线程池停止
                        return stop || !tasks.empty();
                    });
                    if (stop && tasks.empty()) { //任务队列为空并且线程池停止，退出线程
                        return;
                    }
                    task = tasks.front();
                    tasks.pop();
                }
                task();
            }
        }));
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(tasks_mtx);
        stop = true;
    }
    cv.notify_all();
    for (std::thread &th : threads) { // 主线程继续执行之前等待所有其他线程完成
        if (th.joinable()) {
            th.join();
        }
    }
}

void ThreadPool::add(std::function<void()> func) {
    {
        std::unique_lock<std::mutex> lock(tasks_mtx);
        if (stop) {
            throw std::runtime_error("Thread already stop, can't add task any more");
        }
        tasks.emplace(func);
    }
    cv.notify_one();
}