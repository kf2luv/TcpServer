#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

using Task = std::function<void()>;
using Release = std::function<void()>;

// 定时任务类
class TimerTask
{
public:
    TimerTask(int id, int timeout, Task cb)
        : _id(id), _timeout(timeout), _callback(cb) {}

    ~TimerTask()
    {
        // 析构时执行任务
        _callback();
        _release();
    }
    void setRelease(const Release &release)
    {
        _release = release;
    }
    int timeout() const
    {
        return _timeout;
    }

private:
    int _id;          // 任务编号
    int _timeout;     // 定时任务的超时时间
    Task _callback;   // 任务执行的回调函数
    Release _release; // 释放TimerWheel中的定时任务
};

// 时间轮定时器
const size_t MAX_TIMEOUT = 60;
class TimerWheel
{
    using TimerPtr = std::shared_ptr<TimerTask>;
    using TimerWeak = std::weak_ptr<TimerTask>;

public:
    TimerWheel() : _wheel(MAX_TIMEOUT), _tick(0) {};
    // 创建一个定时任务
    void addTimer(int id, int timeout, const Task &cb);
    // 重置一个定时任务
    void resetTimer(int id);
    // 清除一个定时任务
    void removeTimer(int id);

private:
    std::vector<std::vector<TimerPtr>> _wheel;     // 存储定时任务的二维数组
    std::unordered_map<int, TimerWeak> _idToTimer; // 保存定时任务的哈希表
    int _tick;                                     // 秒针
};
