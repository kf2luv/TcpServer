#include "../src/server.hh"
#include <thread>

// uint64_t conn_id = 0;
// std::unordered_map<uint64_t, PtrConnection> conn_map;

// //从Reactor
// // LoopThread conn_loop_thr;

// //主Reactor
// EventLoop base_loop;
// //从Reactor池
// LoopThreadPool loop_thread_pool(&base_loop);

void OnConnected(const PtrConnection& conn)
{
    DF_DEBUG("Connection constructed, id: %d", conn->Id());
}
void MessageHandler(const PtrConnection& conn, Buffer& buf)
{
    //获取消息
    std::string msg = buf.readAsString(buf.readableBytes());
    DF_DEBUG("收到消息: %s", msg.c_str());
    //回显消息
    std::string reply = "Hello, I am server, I recivce your msg : " + msg;
    conn->send(reply.c_str(), reply.size());
}
// void DestroyConnection(const PtrConnection& conn)
// {
//     DF_DEBUG("关闭连接, id: %d", conn->Id());
//     //服务器组件中删除连接
//     conn_map.erase(conn->Id());
// }

// void AcceptHandler(int newfd)
// {
//     ++conn_id;
//     EventLoop* looper = loop_thread_pool.assignLoop();
//     // 创建新连接的channel（注册到event loop中）
//     PtrConnection conn = std::make_shared<Connection>(looper, newfd, conn_id);
//     // 设置新连接各阶段的回调函数
//     conn->setConnectedCallback(OnConnected);
//     conn->setMessageCallback(MessageHandler);
//     conn->setServerClosedCallback(DestroyConnection);
//     // 开启非活跃连接关闭功能
//     conn->enableInactiveClose(6);
//     // 连接准备就绪
//     conn->established();
//     conn_map[conn_id] = conn;
// }

// int main()
// {
//     EventLoop looper;
//     Socket listen_sock;
//     if (!listen_sock.CreateServer(8888))
//     {
//         std::cerr << "Create Server error" << std::endl;
//         return 1;
//     }
//     // 创建监听channel（注册到poller中），可读事件触发时，调用AcceptHandler处理
//     Channel listen_channel(listen_sock.Fd(), &looper);
//     listen_channel.setReadCallback(std::bind(AcceptHandler, &listen_channel, &looper));
//     listen_channel.enableRead();

//     while (true)
//     {
//         looper.start();
//     }

//     return 0;
// }

// int main()
// {
//     EventLoop looper;
//     Acceptor acceptor(8888, &looper, std::bind(AcceptHandler, &looper, std::placeholders::_1));
//     while (true)
//     {
//         looper.start();
//     }
//     return 0;
// }

// int main()
// {
//     EventLoop main_loop;
//     Acceptor acceptor(8888, &main_loop, std::bind(AcceptHandler, std::placeholders::_1));
//     DF_DEBUG("主循环已启动");
//     main_loop.start();
//     return 0;
// }

// int main()
// {
//     loop_thread_pool.setThreadCount(3);
//     loop_thread_pool.start();

//     Acceptor acceptor(8888, &base_loop, std::bind(AcceptHandler, std::placeholders::_1));
//     DF_DEBUG("主循环已启动");
//     base_loop.start();
//     return 0;
// }

int main()
{
    TcpServer server(8888);
    server.setThreadCount(2);
    // server.enableInactiveClose(6);
    server.setMessageCallback(MessageHandler);
    server.setConnectedCallback(OnConnected);
    server.start();

    return 0;
}