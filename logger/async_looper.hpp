#pragma once
#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "buffer.hpp"

namespace ckflogs
{
    /*
        设计思想：两个缓冲区，生产缓冲区和消费缓冲区
                 业务线程（生产者）向生产缓冲区写入日志，工作线程（消费者）从消费缓冲区中读取日志
                 当工作线程检测到消费缓冲区为空且生产缓冲区非空时，交换两个缓冲区
                 当工作线程检测到消费缓冲区为空且生产缓冲区也为空时，等待生产缓冲区数据，再交换两个缓冲区
        设计目的：减少了生产者与消费者之间的锁冲突
    */
    using Functor = std::function<void(Buffer &buffer)>;
    class AsyncLooper
    {
    public:
        enum LooperType
        {
            SAFE,  // 缓冲区固定大小
            UNSAFE // 缓冲区无限扩容
        };

    public:
        AsyncLooper(const Functor &cb, LooperType type = LooperType::SAFE)
            : cb_(cb),
              worker_(std::bind(&AsyncLooper::workerRoutine, this)),
              running_(true),
              type_(type)
        {
            if (type_ == LooperType::UNSAFE)
                std::cout << "unsafe looper create success" << std::endl;
        }
        ~AsyncLooper() { stop(); };

        void stop() // 应该由生产者call
        {
            running_ = false;
            cons_cond_.notify_all(); // 唤醒消费者
            worker_.join();
        }

        void push(const char *str, size_t len)
        {
            if (!running_)
                return;
            {
                std::unique_lock<std::mutex> lck(prod_mtx_);
                if (type_ == LooperType::SAFE)
                {
                    prod_cond_.wait(lck, [&]
                                    { return len <= prod_buffer_.writeAbleSize() || !running_; });
                }
                prod_buffer_.push(str, len);
            }

            cons_cond_.notify_all(); // 唤醒消费者
        }

    private:
        void workerRoutine() // 工作线程的执行函数
        {
            while (true)
            {
                // 若消费缓冲区为空，进行等待交换逻辑
                {
                    std::unique_lock<std::mutex> lck(prod_mtx_);
                    if (!running_ && prod_buffer_.empty())
                        return;
                    // 默认正在运行，如果wait时，检测到运行结束或等待条件成立，则退出wait
                    cons_cond_.wait(lck, [&]
                                    { return !prod_buffer_.empty() || !running_; });
                    cons_buffer_.swap(prod_buffer_);
                }
                if (!cons_buffer_.empty()) // 有可能为空，因为可能wait是因为!running_退出的，换了个空的还是空的
                {
                    cb_(cons_buffer_); // 处理消费缓冲区中的数据
                    if (type_ == SAFE)
                        prod_cond_.notify_all(); // 唤醒生产者
                    cons_buffer_.reset();        // 保证下次从头开始写数据
                }
            }
        }

    private:
        Buffer prod_buffer_;                // 生产缓冲区
        Buffer cons_buffer_;                // 消费缓冲区
        std::mutex prod_mtx_;               // 保证生产者缓冲区的线程安全（操作消费缓冲区的消费者只有一个，不需要锁）
        std::condition_variable prod_cond_; // 生产缓冲区的条件变量，等待消费缓冲区空闲
        std::condition_variable cons_cond_; // 消费缓冲区的条件变量，等待生产缓冲区有数据
        Functor cb_;                        // 由上层决定如何处理缓冲区数据的回调函数
        std::thread worker_;                // 工作线程
        std::atomic<bool> running_;         // 工作线程运行标识 true运行中 false停止运行
        LooperType type_;
    };

}; /*namespace ckflogs*/
