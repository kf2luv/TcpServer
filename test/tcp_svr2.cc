#include "../src/server.hh"
#include <thread>

void HandleClose(Channel *conn_channel)
{
    DF_DEBUG("关闭连接, fd: %d", conn_channel->Fd());
    conn_channel->remove();
    close(conn_channel->Fd());
    delete conn_channel;
}

void HandleRead(Channel *conn_channel)
{
    int fd = conn_channel->Fd();
    // 读取数据
    char buf[64]{};
    int ret = read(fd, buf, 63);
    if (ret <= 0)
    {
        // 关闭连接
        HandleClose(conn_channel);
    }
    else
    {
        DF_DEBUG("收到消息：%s", buf);
        // 收到消息后要回复，开启写事件监控，写事件一旦就绪就可以回复
        conn_channel->enableWrite();
    }

}
void HandleWrite(Channel *conn_channel)
{
    int fd = conn_channel->Fd();
    const char *buf = "SERVER REPLY!!";
    int ret = write(fd, buf, strlen(buf));
    if (ret <= 0)
    {
        // 关闭连接
        HandleClose(conn_channel);
    }
    else
    {
        // 回复完消息不再关心写事件，关闭写事件监控
        DF_DEBUG("回复了一条消息");
        conn_channel->disableWrite();
    }
}

void HandleError(Channel *conn_channel)
{
    HandleClose(conn_channel);
}
void HandleAny(Channel *conn_channel, EventLoop *looper)
{
    DF_DEBUG("有事件发生了");
    
    //活跃连接刷新
    looper->resetTimer(conn_channel->Fd());
}

void HandleInactive(Channel *conn_channel)
{
    // 通知客户非活跃断连
    int fd = conn_channel->Fd();
    const char *msg = "你太久不活动，连接关闭了";
    int ret = send(fd, msg, strlen(msg), 0);
    // 关闭连接
    HandleClose(conn_channel);
}

void AcceptHandler(Channel *listen_channel, EventLoop *looper)
{
    // 获取新连接
    int listen_fd = listen_channel->Fd();
    int newfd = accept(listen_fd, NULL, NULL);
    if (newfd < 0)
    {
        return;
    }
    // 创建新连接的channel（注册到event loop中）
    Channel *conn_channel = new Channel(newfd, looper);
    // 设置新连接各种事件的回调函数
    conn_channel->setReadCallback(std::bind(HandleRead, conn_channel));
    conn_channel->setWriteCallback(std::bind(HandleWrite, conn_channel));
    conn_channel->setErrorCallback(std::bind(HandleError, conn_channel));
    conn_channel->setCloseCallback(std::bind(HandleClose, conn_channel));
    conn_channel->setAnyCallback(std::bind(HandleAny, conn_channel, looper));

    // 非活跃连接超时释放
    looper->addTimer(newfd, 10, std::bind(HandleInactive, conn_channel));

    // 开启读事件监听
    conn_channel->enableRead();
}

void timer_task()
{
    std::cout << "Timer: 新年快乐2025" << std::endl;
}

int main()
{
    EventLoop looper;
    Socket listen_sock;
    if (!listen_sock.CreateServer(8888))
    {
        std::cerr << "Create Server error" << std::endl;
        return 1;
    }
    // 创建监听channel（注册到poller中），可读事件触发时，调用AcceptHandler处理
    Channel listen_channel(listen_sock.Fd(), &looper);
    listen_channel.enableRead();
    listen_channel.setReadCallback(std::bind(AcceptHandler, &listen_channel, &looper));

    looper.addTimer(2025, 5, timer_task);

    while (true)
    {
        looper.start();
    }

    return 0;
}