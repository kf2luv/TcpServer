#include "../src/server.hh"
#include <thread>

void HandleClose(Channel* conn_channel)
{
    DF_DEBUG("关闭连接, fd: %d", conn_channel->Fd());
    conn_channel->remove();
    delete conn_channel;
}

void HandleRead(Channel* conn_channel)
{
    int fd = conn_channel->Fd();
    //读取数据
    char buf[64]{};
    int ret = read(fd, buf, 63);
    if(ret <= 0) {
        //关闭连接
        HandleClose(conn_channel);
    } else {
        DF_DEBUG("收到消息：%s", buf);
        //收到消息后要回复，开启写事件监控，写事件一旦就绪就可以回复
        conn_channel->enableWrite();
    }
}
void HandleWrite(Channel* conn_channel)
{
    int fd = conn_channel->Fd();
    const char* buf = "SERVER REPLY!!";
    int ret = write(fd, buf, strlen(buf));
    if(ret <= 0) {
        //关闭连接
        HandleClose(conn_channel);
    } else {
        //回复完消息不再关心写事件，关闭写事件监控
        DF_DEBUG("回复了一条消息");
        conn_channel->disableWrite();
    }
}

void HandleError(Channel* conn_channel)
{
    HandleClose(conn_channel);
}
void HandleAny(Channel* conn_channel)
{
    DF_DEBUG("有事件发生了");
}


void AcceptHandler(Channel* listen_channel, Poller* poller)
{
    //获取新连接
    int listen_fd = listen_channel->Fd();
    int newfd = accept(listen_fd, NULL,NULL);
    if(newfd < 0){
        return;
    }
    // 创建新连接的channel（注册到poller中）
    Channel *conn_channel = new Channel(newfd, poller);
    // 设置新连接各种事件的回调函数
    conn_channel->setReadCallback(std::bind(HandleRead, conn_channel));
    conn_channel->setWriteCallback(std::bind(HandleWrite, conn_channel));
    conn_channel->setErrorCallback(std::bind(HandleError, conn_channel));
    conn_channel->setCloseCallback(std::bind(HandleClose, conn_channel));
    conn_channel->setAnyCallback(std::bind(HandleAny, conn_channel));

    // 开启读事件监听
    conn_channel->enableRead();
}

int main() {
    Poller poller;
    Socket listen_sock;
    if(!listen_sock.CreateServer(8888))
    {
        std::cerr << "Create Server error" << std::endl;
        return 1;
    }
    //创建监听channel（注册到poller中），可读事件触发时，调用AcceptHandler处理
    Channel listen_channel(listen_sock.Fd(), &poller);
    listen_channel.enableRead();
    listen_channel.setReadCallback(std::bind(AcceptHandler, &listen_channel, &poller));

    std::vector<Channel*> actives;
    while(true) {
        //等待事件发生
        poller.poll(actives);
        //对活跃连接进行事件处理
        for(auto active : actives) {
            active->handleEvent();
        }
    }

    return 0;
}