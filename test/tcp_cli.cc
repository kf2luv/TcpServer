#include "../src/server.hh"

int main()
{
    Socket cli_sock;
    cli_sock.Create();
    cli_sock.Connect("127.0.0.1", 8888);
    std::string msg = "Hello World";

    int ret = cli_sock.NonBlockSend(msg.c_str(), msg.size());
    if (ret <= 0)
    {
        DF_DEBUG("连接已断开");
        return -1;
    }
    else
    {
        DF_DEBUG("发送成功, msg: %s", msg.c_str());
        char buf[64]{};
        ssize_t ret = cli_sock.Recv(buf, 63);
        if (ret <= -1)
        {
            DF_ERROR("连接已断开");
            return 1;
        }
        else
        {
            DF_DEBUG("回显成功：%s", buf);
        }
    }

    return 0;
}