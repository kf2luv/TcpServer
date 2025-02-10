#include "../src/server.hh"

int main()
{
    Socket cli_sock;
    if (!cli_sock.CreateClient("127.0.0.1", 8777))
    {
        return -1;
    }

    // std::string msg = "GET /hi HTTP/1.1\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
    std::string msg = "GET /hi HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    while (1)
    {
        cli_sock.Send(msg.c_str(), msg.size());
        char rd_buf[1024]{};
        assert(cli_sock.Recv(rd_buf, 1023));
        DF_DEBUG("Recv response: %s", rd_buf);
        sleep(2);
    }
    cli_sock.Close();

    return 0;
}