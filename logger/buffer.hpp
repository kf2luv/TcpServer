#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <cassert>

namespace ckflogs
{
    class Buffer
    {
    public:
        const static size_t default_buf_size = 1024;
        const static size_t threshold_buf_size = 1024 * 8;

    public:
        Buffer(size_t buf_size = default_buf_size) : buf_(buf_size), reader_idx_(0), writer_idx_(0) {}
        void push(const char *str, size_t len)
        {
            // 1.确保空间足够
            ensureEnoughSpace(len);
            // 2.向buf_输入数据
            std::copy(str, str + len, &buf_[writer_idx_]);
            writer_idx_ += len;
        }

        void pop(size_t len)
        {
            assert(len <= readAbleSize());
            reader_idx_ += len;
        }

        size_t readAbleSize() // 可读数据大小
        {
            return writer_idx_ - reader_idx_;
        }

        size_t writeAbleSize() // 可写空间大小
        {
            return buf_.size() - writer_idx_;
        }

        void ensureEnoughSpace(size_t len) // 确保写入空间足够
        {
            if (len <= writeAbleSize()) // 空间足够
                return;

            // 空间不足
            // 方案1：固定大小，空间不够则阻塞等待 (实际应用)TODO
            // 方案2：无限扩容 (极限测试用) 二倍扩容，直到某个阈值，线性增长
            size_t old_size = buf_.size();
            size_t new_size = 2 * old_size + len;
            if (new_size >= threshold_buf_size)
            {
                new_size = old_size + len + default_buf_size;
            }
            buf_.resize(new_size);
        }

        char *begin() // 获取读取缓冲区的头部位置
        {
            return &buf_[reader_idx_];
        }

        void swap(Buffer &buffer)
        {
            buf_.swap(buffer.buf_);
            std::swap(reader_idx_, buffer.reader_idx_);
            std::swap(writer_idx_, buffer.writer_idx_);
        }

        void reset()
        {
            reader_idx_ = writer_idx_ = 0;
        }

        size_t size() const
        {
            return buf_.size();
        }

        bool empty() const
        {
            return reader_idx_ == writer_idx_;
        }

    private:
        std::vector<char> buf_; // 存储空间(以字节为单位)
        size_t reader_idx_;     // 读索引, 每次从缓冲区读取数据都从这里开始
        size_t writer_idx_;     // 写索引, 每次向缓冲区写入数据都从这里开始
    };

}; /*namespace ckflogs*/