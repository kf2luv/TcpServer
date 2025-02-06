#include "../src/server.hh"

int main()
{
    Socket cli_sock;
    cli_sock.Create();
    cli_sock.Connect("127.0.0.1", 8888);
    std::string msg = "hello server";
    int cnt = 3;
    while (cnt--)
    {
        sleep(1);
        int ret = cli_sock.NonBlockSend(msg.c_str(), msg.size());
        if (ret < 0)
        {
            break;
        }
        else if (ret == 0)
        {
            continue;
        }
        DF_DEBUG("发送消息：%s", msg.c_str());
    }

    while (1)
    {
        char buf[64]{};
        ssize_t ret = cli_sock.Recv(buf, 63);
        if (ret == -1)
        {
            DF_ERROR("连接已断开");
            return 1;
        }
        else if (ret == 0)
        {
            continue;
        }
        else
        {
            DF_DEBUG("收到消息：%s", buf);
        }
    }
    return 0;
}