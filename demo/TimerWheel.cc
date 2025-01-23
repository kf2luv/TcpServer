#include "TimerWheel.hh"

void TimerWheel::addTimer(int id, int timeout, const TimerTask::Task &cb)
{
    if (_idToTimer.find(id) != _idToTimer.end())
    {
        return;
    }
    // 新建一个TimerTask
    TimerTask *tt = new TimerTask(id, timeout, cb);
    tt->setRelease(std::bind(&TimerWheel::removeTimer, this, id));

    // shared_ptr<TimerTask> 存入时间轮
    TimerPtr ptr(tt); // ptr出了当前作用域会销毁，不影响_wheel中shared_ptr的引用计数
    int pos = (_tick + timeout) % MAX_TIMEOUT;
    _wheel[pos].emplace_back(ptr);

    // weak_ptr<TimerTask> 存入哈希表，作为shared_ptr的副本，后续用这个副本可以获取一个新的shared_ptr
    _idToTimer[id] = TimerWeak(ptr);

    //!!警告!!
    // 直接使用原始资源的指针创建shared_ptr
    // 每次都是建立一个新的shared_ptr，新的计数器，并不是共享计数器
    // 使用weak_ptr解决，weak_ptr.lock()获取的shared_ptr，会增加引用计数
    //  _wheel[pos].push_back(TimerPtr(tt));
}

void TimerWheel::resetTimer(int id)
{
    if (_idToTimer.find(id) == _idToTimer.end())
    {
        // id不存在，无法重置
        return;
    }

    // 根据id所指的weak_ptr，获取一个新的shared_ptr
    TimerPtr ptr = _idToTimer[id].lock();

    // 新的shared_ptr 存入时间轮
    int pos = (_tick + ptr->timeout()) % MAX_TIMEOUT;
    _wheel[pos].push_back(ptr); // 避免push_back增加引用计数?不用，栈变量ptr释放，引用计数--
}

void TimerWheel::removeTimer(int id)
{
    if (_idToTimer.find(id) == _idToTimer.end())
    {
        return;
    }
    _idToTimer.erase(id);
}

void TimerWheel::cancelTimer(int id)
{
    if (_idToTimer.find(id) == _idToTimer.end())
    {
        return;
    }
    TimerPtr ptr = _idToTimer[id].lock();
    ptr->cancel();
}

void TimerWheel::run()
{
    // 移动秒针
    _tick++;
    _tick %= MAX_TIMEOUT;

    // 清空当前秒针位置的数组，释放其中所有任务的shared_ptr
    _wheel[_tick].clear();
}
