#pragma once
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cassert>

class Socket
{
    static const int DEFAULT_BACKLOG = 64;

public:
    Socket() : _sockfd(-1) {}
    ~Socket()
    {
        Close();
    }

    bool Create()
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd < 0)
        {
            return false;
        }

        // int optval = 1;
        // setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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
            return false;
        }

        if (bind(_sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            return false;
        }
        return true;
    }

    bool Listen(int backlog = DEFAULT_BACKLOG)
    {
        if (listen(_sockfd, backlog) < 0)
        {
            return false;
        }
        return true;
    }

    int Accept()
    {
        int fd = accept(_sockfd, NULL, NULL);
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
        return ret == 0;
    }

    void Close()
    {
        if (_sockfd >= 0)
            close(_sockfd);
    }

    int GetSockfd() const
    {
        return _sockfd;
    }

    // 接收数据
    ssize_t Recv(void *buf, size_t len, int flags = 0)
    {
        assert(buf != nullptr);
        ssize_t ret = recv(_sockfd, buf, len, flags);
        if (ret < 0)
        {
        }
        else if (ret == 0)
        {
        }
        else
        {
            return ret;
        }
    }

    // 发送数据
    ssize_t Write(const void *buf, size_t len, int flags)
    {
        assert(buf != nullptr);
        ssize_t ret = send(_sockfd, buf, len, flags);
        if (ret < 0)
        {
        }
        else if (ret == 0)
        {
        }
        else
        {
            return ret;
        }
    }

    // 创建一个server连接
    bool CreateServer(const uint16_t &port, const std::string &ip = "0.0.0.0") 
    {
        Create();
        Bind(ip, port);// "0.0.0.0" -> INADDR_ANY
        Listen();
    }

    // 创建一个client连接
    bool CreateClient(const std::string &ip, const uint16_t &port) 
    {
        Create();
        Connect(ip, port);
    }

private:
    int _sockfd;
};
