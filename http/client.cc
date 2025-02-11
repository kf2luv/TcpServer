#include "http.hh"

// 长短连接测试
void testKeepAlive()
{
    Socket cli_sock;
    if (!cli_sock.CreateClient("127.0.0.1", 8777))
    {
        abort();
    }

    // std::string msg = "GET /hi HTTP/1.1\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
    std::string msg = "GET /hi HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    while (1)
    {
        cli_sock.Send(msg.c_str(), msg.size());
        char rd_buf[1024]{};
        if (!cli_sock.Recv(rd_buf, 1023))
        {
            abort();
        }
        DF_DEBUG("Recv response: %s", rd_buf);
        sleep(2);
    }
    cli_sock.Close();
}

// 非活跃连接测试
void testInactiveClose()
{
    Socket cli_sock;
    if (!cli_sock.CreateClient("127.0.0.1", 8777))
    {
        abort();
    }

    // std::string msg = "GET /hi HTTP/1.1\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
    std::string msg = "GET /hi HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";

    // 发送一次数据,然后就等等看是否会被超时关闭
    cli_sock.Send(msg.c_str(), msg.size());
    char rd_buf[1024]{};
    while (1)
    {
        int ret = cli_sock.Recv(rd_buf, 1023);
        if (ret < 0)
        {
            DF_DEBUG("连接超时关闭了...");
            cli_sock.Close();
            return;
        }
        else if (ret > 0)
        {
            DF_DEBUG("Recv response: %s", rd_buf);
        }
    }
    cli_sock.Close();
}

// 错误请求测试
void testErrorRequest()
{
    Socket cli_sock;
    if (!cli_sock.CreateClient("127.0.0.1", 8777))
    {
        abort();
    }

    // std::string msg = "GET /hi HTTP/1.1\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
    std::string msg = "GET /hi HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 100\r\n\r\nhelloworld";
    cli_sock.Send(msg.c_str(), msg.size());

    // 只发送一次: 服务器收不到正确的报文 (body实际长度 < Content-Length), 也就无法进行业务处理, 最终导致连接超时关闭

    // 发送多次: 第一次请求会把后面的请求当成它的正文
    cli_sock.Send(msg.c_str(), msg.size());
    cli_sock.Send(msg.c_str(), msg.size());
    cli_sock.Send(msg.c_str(), msg.size());
    cli_sock.Send(msg.c_str(), msg.size());
    cli_sock.Send(msg.c_str(), msg.size());
    cli_sock.Send(msg.c_str(), msg.size());

    while (1)
    {
        char rd_buf[1024]{};
        int ret = cli_sock.Recv(rd_buf, 1023);
        if (ret < 0)
        {
            DF_DEBUG("连接关闭了...");
            cli_sock.Close();
            return;
        }
        else if (ret > 0)
        {
            DF_DEBUG("Recv response: %s", rd_buf);
        }
    }
    cli_sock.Close();
}

// 业务处理超时测试
void testLongBusiness()
{
    for (int i = 0; i < 9; i++)
    {
        pid_t fd = fork();
        if (fd < 0)
        {
            return;
        }
        else if (fd == 0)
        {
            // 创建客户端连接
            Socket cli_sock;
            if (!cli_sock.CreateClient("127.0.0.1", 8777))
            {
                return;
            }
            DF_DEBUG("创建客户端 %d", i + 1);
            std::string msg = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
            while (1)
            {
                assert(cli_sock.Send(msg.c_str(), msg.size()) > 0);

                char rd_buf[1024]{};
                int ret = cli_sock.Recv(rd_buf, 1023);
                if (ret < 0)
                {
                    DF_DEBUG("Connection is closed...");
                    cli_sock.Close();
                    return;
                }
                else if (ret > 0)
                {
                    DF_DEBUG("Recv response: %s", rd_buf);
                }
            }
            cli_sock.Close();
            exit(1);
        }
    }
    // server有三个工作线程
    sleep(999);
}

// 测试一次发送多条请求的
void testManyRequestOnce()
{
    Socket cli_sock;
    if (!cli_sock.CreateClient("127.0.0.1", 8777))
    {
        abort();
    }

    // std::string msg = "GET /hi HTTP/1.1\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
    std::string msg = "GET /hi HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    msg += "GET /hi HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    msg += "GET /hi HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    msg += "GET /hi HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";

    cli_sock.Send(msg.c_str(), msg.size());

    while (1)
    {
        char rd_buf[1024]{};
        int ret = cli_sock.Recv(rd_buf, 1023);
        if (ret < 0)
        {
            DF_DEBUG("连接关闭了...");
            cli_sock.Close();
            return;
        }
        else if (ret > 0)
        {
            DF_DEBUG("Recv response: %s", rd_buf);
        }
    }
    cli_sock.Close();
}

// 大文件上传测试
void testUploadBigFile()
{
    Socket cli_sock;
    if (!cli_sock.CreateClient("127.0.0.1", 8777))
    {
        abort();
    }

    std::string msg = "PUT /upload/big.txt HTTP/1.1\r\nConnection: keep-alive\r\n";
    std::string body;
    DF_DEBUG("读取文件数据中...");
    assert(Util::readFile("./big.txt", body) == true);
    msg += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
    msg += body;
    DF_DEBUG("读取完成, 发送上传请求");

    cli_sock.Send(msg.c_str(), msg.size());

    while (1)
    {
        char rd_buf[1024]{};
        int ret = cli_sock.Recv(rd_buf, 1023);
        if (ret < 0)
        {
            DF_DEBUG("连接关闭了...");
            cli_sock.Close();
            return;
        }
        else if (ret > 0)
        {
            DF_DEBUG("Recv response: %s", rd_buf);
        }

        sleep(5);
    }
    cli_sock.Close();
}

int main()
{
    // testInactiveClose();
    // testErrorRequest();
    // testLongBusiness();
    // testManyRequestOnce();
    testUploadBigFile();
    return 0;
}