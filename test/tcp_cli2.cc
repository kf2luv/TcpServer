#include "../src/server.hh"

int main()
{
    Socket cli_sock;
    cli_sock.Create();
    cli_sock.Connect("127.0.0.1", 8888);
    std::string msg = "hello";
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
        else
        {
        }
    }

    while (1)
    {
        char buf[64]{};
        ssize_t ret = cli_sock.Recv(buf, 63);
        if (ret == -1)
        {
            std::cout << "连接已断开" << std::endl;
            return 1;
        }
        else if (ret == 0)
        {
            continue;
        }
        else
        {
            std::cout << buf << std::endl;
        }
    }

    return 0;
}