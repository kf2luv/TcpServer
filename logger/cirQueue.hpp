#pragma once
#include <iostream>
#include <vector>
#include <semaphore.h>
#include <pthread.h>

static const int g_size = 500;

template <class T>
class cirQueue
{
public:
    cirQueue(int size = g_size) : _cq(size), _comsumer(0), _productor(0)
    {
        sem_init(&_sem_room, 0, size);
        sem_init(&_sem_data, 0, 0);

        pthread_mutex_init(&_c_mutex,nullptr);
        pthread_mutex_init(&_p_mutex,nullptr);
    }

    // productor call
    void push(const T &in)
    {
        // 申请空间信号量
        sem_wait(&_sem_room);

        // 输入数据（占用空间）
        // 保证生产者——生产者的互斥
        pthread_mutex_lock(&_p_mutex);
        _cq[_productor] = in;
        _productor = (_productor + 1) % _cq.size();
        pthread_mutex_unlock(&_p_mutex);

        // 增加数据信号量
        sem_post(&_sem_data);
    }

    // consumer call
    void pop(T *out)
    {
        // 申请数据信号量
        sem_wait(&_sem_data);

        // 输出数据（释放空间）
        // 保证消费者——消费者的互斥
        pthread_mutex_lock(&_c_mutex);
        *out = _cq[_comsumer];
        _comsumer = (_comsumer + 1) % _cq.size();
        pthread_mutex_unlock(&_c_mutex);

        // 增加空间信号量
        sem_post(&_sem_room);
    }

    ~cirQueue()
    {
        sem_destroy(&_sem_room);
        sem_destroy(&_sem_data);

        pthread_mutex_destroy(&_c_mutex);
        pthread_mutex_destroy(&_p_mutex);
    }

private:
    std::vector<T> _cq;
    int _comsumer;
    int _productor;

    pthread_mutex_t _c_mutex;
    pthread_mutex_t _p_mutex;

    sem_t _sem_room; // 空间信号量
    sem_t _sem_data; // 资源信号量
};