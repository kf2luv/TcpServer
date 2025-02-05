#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <memory>

#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#include "../logger/ckflog.hpp"

/*

    Buffer缓冲区模块

*/
class Buffer
{
    using byte = char;
    static const size_t DEFAULT_BUF_SIZE = 1024;

public:
    Buffer(size_t size = DEFAULT_BUF_SIZE)
        : _buffer(size), _read_idx(0), _write_idx(0) {}

    /// @brief 向缓冲区写入数据
    /// @param data 写入数据起始地址，统一转成void*就不用管类型
    /// @param len 待写入的数据字节数
    void write(const void *data, size_t len)
    {
        // 1.确保缓冲区的写入空间充足
        ensureEnoughWriteSpace(len);

        // 2.写入数据
        const byte *source = (const byte *)data;
        std::copy(source, source + len, writePos());

        // 3.移动写偏移
        _write_idx += len;
        assert(_write_idx <= _buffer.size());
    }

    void writeString(const std::string &str)
    {
        write(str.c_str(), str.size());
    }

    void writeBuffer(Buffer &buf)
    {
        write(buf.readPos(), buf.readableBytes());
    }

    /// @brief 从缓冲区读取数据，要求获取的len必须小于可读数据字节数
    /// @param data 读取数据起始地址
    /// @param len 待读取的数据字节数
    void read(void *data, size_t len)
    {
        assert(len <= readableBytes());

        // 1.读取数据
        std::copy(readPos(), readPos() + len, (char *)data);

        // 2.移动读偏移
        _read_idx += len;
        assert(_read_idx <= _buffer.size());
    }

    // 获取一个长度为len的字符串
    std::string readAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string str;
        str.resize(len);
        read(&str[0], len);
        return str;
    }

    // 获取读取一行（以/n结尾）
    std::string getLine()
    {
        byte *pos = (char *)memchr(readPos(), '\n', readableBytes());
        if (pos == NULL)
        {
            return "";
        }
        //+1把\n也取出来
        return readAsString(pos - readPos() + 1);
    }

    // 清理缓冲区
    void clear()
    {
        _read_idx = _write_idx = 0;
    }

    // 判断缓冲区是否为空
    bool empty()
    {
        return readableBytes() == 0;
    }

    // 获取可读数据大小
    size_t readableBytes()
    {
        return _write_idx - _read_idx;
    }

    byte *begin()
    {
        return &_buffer[0];
    }

    byte *writePos()
    {
        return begin() + _write_idx;
    }

    byte *readPos()
    {
        return begin() + _read_idx;
    }

private:
    // 确保缓冲区中的剩余空间可以写入len个数据
    void ensureEnoughWriteSpace(size_t len)
    {
        // 1.判断`write_idx`后的剩余空间是否足够
        size_t backFreeSpace = _buffer.size() - _write_idx;
        if (backFreeSpace >= len)
        {
            return;
        }

        // 2.判断总体的剩余空间是否足够
        if (_read_idx + backFreeSpace >= len)
        {
            std::cout << "挪动了一次数据" << std::endl;
            // 将数据挪到起始位置，读写偏移要跟着移动
            size_t rbytes = readableBytes();
            std::copy(readPos(), readPos() + rbytes, begin());
            _read_idx = 0;
            _write_idx = rbytes;

            return;
        }

        // 3.扩容，保证写偏移之后有足够的空间
        std::cout << "扩容了一次" << std::endl;
        size_t newSize = _write_idx + len;
        _buffer.resize(newSize);
    }

private:
    std::vector<byte> _buffer; // 缓冲区空间（以字节为单位）
    size_t _read_idx;          // 读偏移
    size_t _write_idx;         // 写偏移
};

/*

    Socket套接字模块

*/
class Socket
{
    static const int DEFAULT_BACKLOG = 64;

public:
    Socket() : _sockfd(-1) {}
    ~Socket()
    {
        Close();
    }
    Socket(int fd) : _sockfd(fd) {}

    bool Create()
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd < 0)
        {
            DF_FATAL("Socket create failed: %s", strerror(errno));
            return false;
        }

        _sockfd = sockfd;
        return true;
    }

    bool Bind(const std::string &ip, const uint16_t &port)
    {
        struct sockaddr_in sin;
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);

        int ret = inet_pton(AF_INET, ip.c_str(), &sin.sin_addr.s_addr);
        if (ret != 1)
        {
            DF_ERROR("ip error");
            return false;
        }

        if (bind(_sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            DF_FATAL("Socket bind failed: %s", strerror(errno));
            return false;
        }
        return true;
    }

    bool Listen(int backlog = DEFAULT_BACKLOG)
    {
        if (listen(_sockfd, backlog) < 0)
        {
            DF_FATAL("Socket listen failed: %s", strerror(errno));
            return false;
        }
        return true;
    }

    // 返回-2表示还可以重新accpet
    // 返回-1表示accpet异常
    int Accept()
    {
        int fd = accept(_sockfd, NULL, NULL);
        if (fd < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                return -2;
            }
            DF_ERROR("Accept new fd failed: %s", strerror(errno));
            return -1;
        }
        return fd;
    }

    bool Connect(const std::string &svr_ip, const uint16_t &svr_port)
    {
        struct sockaddr_in svr;
        svr.sin_family = AF_INET;
        svr.sin_port = htons(svr_port);
        if (inet_pton(AF_INET, svr_ip.c_str(), &svr.sin_addr.s_addr) != 1)
        {
            return false;
        }

        int ret = connect(_sockfd, (struct sockaddr *)&svr, sizeof(svr));
        if (ret < 0)
        {
            DF_ERROR("Connect failed: %s", strerror(errno));
            return false;
        }
        return true;
    }

    void Close()
    {
        if (_sockfd >= 0)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }

    int Fd() const
    {
        return _sockfd;
    }

    // 接收数据（默认是阻塞接收）
    ssize_t Recv(void *buf, size_t len, int flags = 0)
    {
        assert(buf != nullptr);
        ssize_t ret = recv(_sockfd, buf, len, flags);

        if (ret <= 0)
        {
            // 可以重新接收数据，返回0
            if (errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
            // 接收出错 or 连接断开，不能重新接收数据，返回-1
            DF_ERROR("Recv from fd-%d failed: %s", _sockfd, strerror(errno));
            return -1;
        }
        // 数据接收成功
        return ret;
    }
    // 非阻塞接收
    ssize_t NonBlockRecv(void *buf, size_t len)
    {
        return Recv(buf, len, MSG_DONTWAIT);
    }

    // 发送数据
    ssize_t Send(const void *buf, size_t len, int flags = 0)
    {
        assert(buf != nullptr);
        ssize_t ret = send(_sockfd, buf, len, flags);
        if (ret < 0)
        {
            // 可以重新发送数据，返回0
            if (errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
            // 发送出错 or 连接断开，不能重新发送数据，返回-1
            DF_ERROR("Send toto fd-%d failed: %s", _sockfd, strerror(errno));
            return -1;
        }
        // 数据发送成功
        return ret;
    }
    ssize_t NonBlockSend(const void *buf, size_t len)
    {
        return Send(buf, len, MSG_DONTWAIT);
    }

    // 创建一个server连接
    bool CreateServer(const uint16_t &port, const std::string &ip = "0.0.0.0")
    {
        return Create() && SetNonBlock() && SetAddrReuse() && Bind(ip, port) && Listen();
    }

    // 创建一个client连接
    bool CreateClient(const std::string &ip, const uint16_t &port)
    {
        return Create() && Connect(ip, port);
    }

    // 设置套接字为地址重用
    bool SetAddrReuse()
    {
        int optval = 1;
        if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        {
            DF_ERROR("fd-%d SetAddrReuse", _sockfd);
            return false;
        }
        return true;
    }

    // 设置套接字为非阻塞
    bool SetNonBlock()
    {
        int flags = fcntl(_sockfd, F_GETFL, 0);
        if (fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            DF_ERROR("fd-%d SetNonBlock", _sockfd);
            return false;
        }
        return true;
    }

private:
    int _sockfd;
};

/*

    Channel事件管理器

*/
class Poller;
class EventLoop;
class Channel
{
    using EventCallback = std::function<void()>; // 事件回调函数类型
public:
    Channel(int fd, EventLoop* looper) : _fd(fd), _looper(looper) {}

    // EventLoop监控到事件发生后，调用此函数
    void setREvents(uint32_t revents)
    {
        _revents = revents;
    }

    int Fd() const
    {
        return _fd;
    }

    uint32_t Events() const
    {
        return _events;
    }

    void setReadCallback(const EventCallback &event_cb)
    {
        _read_callback = event_cb;
    }
    void setWriteCallback(const EventCallback &event_cb)
    {
        _write_callback = event_cb;
    }
    void setErrorCallback(const EventCallback &event_cb)
    {
        _error_callback = event_cb;
    }
    void setCloseCallback(const EventCallback &event_cb)
    {
        _close_callback = event_cb;
    }
    void setAnyCallback(const EventCallback &event_cb)
    {
        _any_callback = event_cb;
    }

    // 启动可读事件监控
    void enableRead()
    {
        _events |= EPOLLIN;
        // 添加到EventLoop的事件监控中 TODO
        update();
    }
    // 启动可写事件监控
    void enableWrite()
    {
        _events |= EPOLLOUT;
        update();
    }
    // 关闭可读事件监控
    void disableRead()
    {
        _events &= ~EPOLLIN;
        update();
    }
    // 关闭可写事件监控
    void disableWrite()
    {
        _events &= ~EPOLLOUT;
        update();
    }
    // 关闭所有事件监控
    void disableAll()
    {
        _events = 0;
        update();
    }
    // 是否监控了读事件
    bool isReadAble()
    {
        return (_events & EPOLLIN);
    }
    // 是否监控了写事件
    bool isWriteAble()
    {
        return (_events & EPOLLOUT);
    }

    // 向事件监控器更新当前Channel关心的事件（当前事件监控器是Poller，后续要集成到EventLoop中）
    void update();
    // 从事件监控器删除当前Channel的监控
    void remove();

    // 事件处理，描述符触发事件，由这个函数分辨是哪种事件并调用相应的回调函数
    // 如果在回调中关闭了连接，后续事件处理可能会因为资源无效而出问题
    void handleEvent()
    { 
        // 任意事件发生
        if (_any_callback)
        {
            DF_DEBUG("revents: %d", _revents);
            _any_callback();
        }
        // 可读事件发生 （正常地收到可读数据 or 对端关闭写端或连接时 or 收到带外数据）
        if ((_revents & EPOLLIN) || (_revents & EPOLLRDHUP) || (_revents & EPOLLPRI))
        {
            if (_read_callback)
            {
                _read_callback();
            }
        }
        // 可写事件发生（写数据时可能发现对端关闭了连接，发不过去，此时本地也要关闭连接，因此可能会导致连接关闭）
        else if (_revents & EPOLLOUT)
        {
            if (_write_callback)
            {
                _write_callback();
            }
        }
        // 有可能导致连接关闭的事件处理，一次只执行一个
        // 错误事件发生
        else if (_revents & EPOLLERR)
        {
            if (_error_callback)
            {
                _error_callback();
            }
        }
        // 关闭连接事件发生
        else if (_revents & EPOLLHUP)
        {
            if (_close_callback)
            {
                _close_callback();
            }
        }
    }

private:
    int _fd;           // 管理的描述符
    uint32_t _events;  // 描述符所关心的事件
    uint32_t _revents; // 当前描述符触发的事件

    EventCallback _read_callback;
    EventCallback _write_callback;
    EventCallback _error_callback;
    EventCallback _close_callback;
    EventCallback _any_callback;

    EventLoop *_looper; // 事件监控器(后续改为EventLoop)
};

/*

    Poller: 事件监控器

*/
class Poller
{
    const static size_t MAX_EVENTS = 64;
    const static int WAIT_TIMEOUT = -1;

public:
    Poller()
    {
        _epfd = epoll_create(MAX_EVENTS);
        if (_epfd < 0)
        {
            DF_ERROR("Epoll create failed, %s", strerror(errno));
            abort();
        }
    }
    ~Poller()
    {
        close(_epfd);
    }
    // 添加或修改监控事件
    bool updateEvent(Channel *channel)
    {
        assert(channel);
        // 存在, Modify；不存在, Add
        int op = hasChannel(channel) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        if (!epollControlHelper(op, channel))
        {
            return false;
        }
        // 修改描述符与channel的映射关系
        _fdToChannel[channel->Fd()] = channel;
        return true;
    }

    // 移除监控事件
    bool removeEvent(Channel *channel)
    {
        assert(channel);
        int fd = channel->Fd();
        if (!hasChannel(channel))
        {
            // 并没有对此fd进行监控，不用移除
            return true;
        }
        // 1.从epoll红黑树中删除
        if (!epollControlHelper(EPOLL_CTL_DEL, channel))
        {
            return false;
        }
        // 2.从哈希映射中删除
        _fdToChannel.erase(fd);
        return true;
    }

    // 开始监控，返回活跃Channel
    void poll(std::vector<Channel *>& actives)
    {
        // 等待epoll事件发生
        int nfds = epoll_wait(_epfd, _events, MAX_EVENTS, WAIT_TIMEOUT);
        if (nfds < 0)
        {
            if(errno == EINTR)
            {
                return;
            }
            DF_ERROR("Epoll wait failed: %s", strerror(errno));
            abort();
        }
        // 记录并返回活跃的Channel
        actives.clear();
        actives.resize(nfds);
        for (int i = 0; i < nfds; i++)
        {
            auto it = _fdToChannel.find(_events[i].data.fd);
            assert(it != _fdToChannel.end());

            //添加actives
            actives[i] = it->second;
            //设置Channel的就绪事件
            actives[i]->setREvents(_events[i].events);
        }
    }

private:
    bool epollControlHelper(int op, Channel *channel)
    {
        int fd = channel->Fd();
        struct epoll_event event;
        event.events = channel->Events();
        event.data.fd = fd;

        int ret = epoll_ctl(_epfd, op, fd, &event);
        if (ret < 0)
        {
            DF_ERROR("Epoll Control failed, fd: %d, error: %s", fd, strerror(errno));
            return false;
        }
        return true;
    }

    bool hasChannel(const Channel *channel)
    {
        return _fdToChannel.find(channel->Fd()) != _fdToChannel.end();
    }

private:
    int _epfd;                                       // epoll操作句柄
    struct epoll_event _events[MAX_EVENTS];          // 保存内核监控到的活跃事件
    std::unordered_map<int, Channel *> _fdToChannel; // 描述符fd和channel的映射关系
};


/*

    Timer：定时器模块

*/
class TimerTask
{
public:
    using Task = std::function<void()>;
    using Release = std::function<void()>;

    TimerTask(int id, int timeout, Task cb)
        : _id(id), _timeout(timeout), _callback(cb), _cancelled(false) {}

    ~TimerTask()
    {
        if (!_cancelled)
        {
            // 如果没有被取消，析构时执行任务
            _callback();
            _release();
        }
    }
    void setRelease(const Release &release)
    {
        _release = release;
    }
    int timeout() const
    {
        return _timeout;
    }
    void cancel()
    {
        _cancelled = true;
    }

private:
    int _id;          // 任务编号
    int _timeout;     // 定时任务的超时时间
    Task _callback;   // 任务执行的回调函数
    Release _release; // 释放TimerWheel中的定时任务
    bool _cancelled;  //任务是否被取消
};
class TimerWheel
{
    static const size_t MAX_TIMEOUT = 60;
    using TimerPtr = std::shared_ptr<TimerTask>;
    using TimerWeak = std::weak_ptr<TimerTask>;

public:
    TimerWheel(EventLoop *looper)
        : _wheel(MAX_TIMEOUT), _tick(0), _looper(looper)
        , _timerfd(createTimerFd()), _timer_channel(std::make_unique<Channel>(_timerfd, _looper))
    {
        // 为定时器设置到期任务回调函数
        _timer_channel->setReadCallback(std::bind(&TimerWheel::onTime, this));
        // 启动timerfd的读事件监控
        _timer_channel->enableRead();
    };

    // 创建一个定时任务
    void addTimer(int id, int timeout, const TimerTask::Task& cb);
    // 重置一个定时任务
    void resetTimer(int id);
    // 取消一个定时任务
    void cancelTimer(int id);

    // 运行时间轮一次，即执行秒针指向位置的所有到期任务，随后指针向后偏移一位
    void runTimerTask()
    {
        // 移动秒针
        _tick++;
        _tick %= MAX_TIMEOUT;

        // 清空当前秒针位置的数组，释放其中所有任务的shared_ptr
        _wheel[_tick].clear();
    }

    //判断定时任务是否存在（存在线程安全问题，只能在EventLoop当前线程调用）
    bool hasTimer(int id)
    {
        return _idToTimer.find(id) != _idToTimer.end();
    }

private:
    void addTimerInLoop(int id, int timeout, const TimerTask::Task &cb)
    {
        if (hasTimer(id))
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

        // !!警告!!
        // 直接使用原始资源的指针创建shared_ptr
        // 每次都是建立一个新的shared_ptr，新的计数器，并不是共享计数器
        // 使用weak_ptr解决，weak_ptr.lock()获取的shared_ptr，会增加引用计数
        //  _wheel[pos].push_back(TimerPtr(tt));
    }
    void resetTimerInLoop(int id)
    {
        if (!hasTimer(id))
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
    void cancelTimerInloop(int id)
    {
        if (!hasTimer(id))
        {
            return;
        }
        TimerPtr ptr = _idToTimer[id].lock();
        ptr->cancel();
    }
    // 清除定时任务
    void removeTimer(int id)
    {
        if (!hasTimer(id))
        {
            return;
        }
        _idToTimer.erase(id);
    }

    // 创建一个timerfd
    static int createTimerFd()
    {
        // 创建 timerfd
        int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (tfd == -1)
        {
            DF_ERROR("Timerfd create failed");
            abort();
        }

        // 设置定时器, 每隔 1 秒，超时一次
        struct itimerspec its;
        its.it_value.tv_sec = 1; // 第一次超时的时间：1 秒后
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = 1; // 往后间隔时间：1 秒
        its.it_interval.tv_nsec = 0;

        if (timerfd_settime(tfd, 0, &its, NULL) == -1)
        {
            DF_ERROR("Timerfd set time failed");
            abort();
        }
        return tfd;
    }

    // 读取清空timerfd的数据
    void readTimerFd()
    {
        uint64_t expiration;
        ssize_t ret = read(_timerfd, &expiration, sizeof(uint64_t));
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                return;
            }
            DF_ERROR("Timerfd read failed");
            return;
        }
    }

    // 定时器超时处理的回调函数
    void onTime()
    {
        // 处理timerfd的读事件，把数据读掉，以防一次超时多次处理
        readTimerFd();
        // 超时一次，运行一次时间轮
        runTimerTask();
    }

private:
    std::vector<std::vector<TimerPtr>> _wheel;     // 存储定时任务的二维数组
    std::unordered_map<int, TimerWeak> _idToTimer; // 保存定时任务的哈希表
    int _tick;                                     // 秒针

    EventLoop *_looper;                      // 绑定的event loop
    int _timerfd;                            // 用于超时事件监控
    std::unique_ptr<Channel> _timer_channel; // timerfd的channel
};

class EventLoop
{
    using Task = std::function<void()>;

public:
    EventLoop()
        : _eventfd(createEventFd())
        , _event_channel(std::make_unique<Channel>(_eventfd, this))
        , _thread_id(std::this_thread::get_id())
        , _timer_wheel(this)
    {
        // 给eventfd添加读事件的回调函数
        _event_channel->setReadCallback(std::bind(&EventLoop::readEventFd, this));
        // 开启eventfd读事件监听
        _event_channel->enableRead();
    }

    void start()
    {
        // 1.IO事件监听 (可能会被eventfd唤醒，此时应跳到第3步)
        std::vector<Channel *> actives;
        _poller.poll(actives);

        // 2.事件处理
        if (!actives.empty())
        {
            for (auto &a : actives)
            {
                a->handleEvent();
            }
        }

        // 3.任务执行（其它线程的、延后处理的）
        runAllTasks();
    }

    // 判断当前线程是否是EventLoop所绑定的线程
    bool isInLoop()
    {
        return std::this_thread::get_id() == _thread_id;
    }
    // 在这个EventLoop中执行任务 （其它线程可调用）
    // EventLoop本线程任务：直接执行
    // 其它线程任务：压入任务队列
    void runInLoop(const Task &cb)
    {
        if (isInLoop())
        {
            cb();
        }
        else
        {
            cacheTask(cb);
        }
    }

    // 将任务暂时缓存到任务队列
    void cacheTask(const Task &cb)
    {
        {
            std::unique_lock<std::mutex> lockguard(_mtx);
            _tasks.push_back(cb);
        }
        weakupEventFd();
    }

    // 新增/修改监控事件
    bool updateEvent(Channel *channel)
    {
        return _poller.updateEvent(channel);
    }

    // 移除监控事件
    bool removeEvent(Channel *channel)
    {
        return _poller.removeEvent(channel);
    }

    // 新增定时任务
    void addTimer(int id, int timeout, TimerTask::Task cb)
    {
        _timer_wheel.addTimer(id, timeout, cb);
    }
    // 重置定时任务
    void resetTimer(int id)
    {
        _timer_wheel.resetTimer(id);
    }
    // 取消一个定时任务
    void cancelTimer(int id)
    {
        _timer_wheel.cancelTimer(id);
    }

    bool hasTimer(int id)
    {
        return _timer_wheel.hasTimer(id);
    }

private:
    void runAllTasks()
    {
        // 取出任务池中的所有任务，执行
        std::vector<Task> tasks;
        {
            std::unique_lock<std::mutex> lockguard(_mtx);
            if(_tasks.empty())
            {
                return;
            }
            tasks.swap(_tasks);
        }
        for (auto &t : tasks)
        {
            t();
        }
    }

    static int createEventFd()
    {
        int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (efd < 0)
        {
            DF_ERROR("Eventfd create failed");
            abort();
        }
        return efd;
    }

    void readEventFd()
    {
        uint64_t signal = 0;
        int ret = read(_eventfd, &signal, sizeof(uint64_t));
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                return;
            }
            DF_ERROR("Eventfd read failed");
            return;
        }
    }

    void weakupEventFd()
    {
        uint64_t signal = 1;
        int ret = write(_eventfd, &signal, sizeof(uint64_t));
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                return;
            }
            DF_ERROR("Eventfd write failed");
            return;
        }
    }

private:
    std::thread::id _thread_id; // 事件循环所在线程id
    int _eventfd;               // 用于唤醒IO事件监听阻塞
                                // （向eventfd计数器写入，就有了一个读事件，就可以唤醒了），先去执行任务
    std::unique_ptr<Channel> _event_channel; // eventfd对应的channel
    Poller _poller;                          // 事件监听器
    std::vector<Task> _tasks;                // 任务池
    std::mutex _mtx;                         // 保护任务池
    TimerWheel _timer_wheel;                 // 定时器
};

void Channel::update()
{
    _looper->updateEvent(this);
}

void Channel::remove()
{
    _looper->removeEvent(this);
}

// TimerWheel创建一个定时任务
void TimerWheel::addTimer(int id, int timeout, const TimerTask::Task &cb)
{
    _looper->runInLoop(std::bind(&TimerWheel::addTimerInLoop, this, id, timeout, cb));
}

// TimerWheel重置一个定时任务
void TimerWheel::resetTimer(int id)
{
    _looper->runInLoop(std::bind(&TimerWheel::resetTimerInLoop, this, id));
}

// TimerWheel取消一个定时任务
void TimerWheel::cancelTimer(int id)
{
    _looper->runInLoop(std::bind(&TimerWheel::cancelTimerInloop, this, id));
}

