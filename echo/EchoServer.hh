#include "../src/server.hh"

class EchoServer
{
private:
    TcpServer _server;

private:
    void OnConnected(const PtrConnection &conn)
    {
        DF_DEBUG("新建连接, id: %d", conn->Id());
    }
    void OnClosed(const PtrConnection &conn)
    {
        DF_DEBUG("关闭连接, id: %d", conn->Id());
    }
    void MessageHandler(const PtrConnection &conn, Buffer &buf)
    {
        // 获取消息
        std::string msg = buf.readAsString(buf.readableBytes());
        // 回显消息
        conn->send(msg.c_str(), msg.size());
        DF_DEBUG("ECHO SUCCESSED, CLIENT ID: %d", conn->Id());
        // here
        // 关闭客户端
        conn->shutdown();
    }

public:
    EchoServer(uint32_t port, size_t thread_count = 10) : _server(port)
    {
        _server.setThreadCount(thread_count);
        // 设置回调函数
        _server.setConnectedCallback(std::bind(&EchoServer::OnConnected, this, std::placeholders::_1));
        _server.setMessageCallback(std::bind(&EchoServer::MessageHandler, this, std::placeholders::_1, std::placeholders::_2));
        _server.setClosedCallback(std::bind(&EchoServer::OnClosed, this, std::placeholders::_1));
    }
    void Start()
    {
        _server.start();
    }
};