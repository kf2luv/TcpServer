#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../logger/ckflog.hpp"

/*

    Buffer缓冲区模块

*/
class Buffer
{
    using byte = char;
    static const size_t DEFAULT_BUF_SIZE = 1024;

public:
    Buffer(size_t size = DEFAULT_BUF_SIZE)
        : _buffer(size), _read_idx(0), _write_idx(0) {}

    /// @brief 向缓冲区写入数据
    /// @param data 写入数据起始地址，统一转成void*就不用管类型
    /// @param len 待写入的数据字节数
    void write(const void *data, size_t len);

    void writeString(const std::string &str);
    void writeBuffer(Buffer &buf);

    /// @brief 从缓冲区读取数据，要求获取的len必须小于可读数据字节数
    /// @param data 读取数据起始地址
    /// @param len 待读取的数据字节数
    void read(void *data, size_t len);
    // 获取一个长度为len的字符串
    std::string readAsString(size_t len);
    // 获取读取一行（以/n结尾）
    std::string getLine();

    // 清理缓冲区
    void clear();
    // 判断缓冲区是否为空
    bool empty();
    // 获取可读数据大小
    size_t readableBytes();

    byte *begin();
    byte *writePos();
    byte *readPos();

private:
    // 确保缓冲区中的剩余空间可以写入len个数据
    void ensureEnoughWriteSpace(size_t len);

private:
    std::vector<byte> _buffer; // 缓冲区空间（以字节为单位）
    size_t _read_idx;          // 读偏移
    size_t _write_idx;         // 写偏移
};

void Buffer::write(const void *data, size_t len)
{
    // 1.确保缓冲区的写入空间充足
    ensureEnoughWriteSpace(len);

    // 2.写入数据
    const byte *source = (const byte *)data;
    std::copy(source, source + len, writePos());

    // 3.移动写偏移
    _write_idx += len;
    assert(_write_idx <= _buffer.size());
}

void Buffer::writeString(const std::string &str)
{
    write(str.c_str(), str.size());
}

void Buffer::writeBuffer(Buffer &buf)
{
    write(buf.readPos(), buf.readableBytes());
}

void Buffer::read(void *data, size_t len)
{
    assert(len <= readableBytes());

    // 1.读取数据
    std::copy(readPos(), readPos() + len, (char *)data);

    // 2.移动读偏移
    _read_idx += len;
    assert(_read_idx <= _buffer.size());
}

void Buffer::clear()
{
    _read_idx = _write_idx = 0;
}

bool Buffer::empty()
{
    return readableBytes() == 0;
}

size_t Buffer::readableBytes()
{
    return _write_idx - _read_idx;
}

void Buffer::ensureEnoughWriteSpace(size_t len)
{
    // 1.判断`write_idx`后的剩余空间是否足够
    size_t backFreeSpace = _buffer.size() - _write_idx;
    if (backFreeSpace >= len)
    {
        return;
    }

    // 2.判断总体的剩余空间是否足够
    if (_read_idx + backFreeSpace >= len)
    {
        std::cout << "挪动了一次数据" << std::endl;
        // 将数据挪到起始位置，读写偏移要跟着移动
        size_t rbytes = readableBytes();
        std::copy(readPos(), readPos() + rbytes, begin());
        _read_idx = 0;
        _write_idx = rbytes;

        return;
    }

    // 3.扩容，保证写偏移之后有足够的空间
    std::cout << "扩容了一次" << std::endl;
    size_t newSize = _write_idx + len;
    _buffer.resize(newSize);
}

Buffer::byte *Buffer::begin()
{
    return &_buffer[0];
}

Buffer::byte *Buffer::writePos()
{
    return begin() + _write_idx;
}

Buffer::byte *Buffer::readPos()
{
    return begin() + _read_idx;
}

std::string Buffer::readAsString(size_t len)
{
    assert(len <= readableBytes());
    std::string str;
    str.resize(len);
    read(&str[0], len);
    return str;
}

std::string Buffer::getLine()
{
    byte *pos = (char *)memchr(readPos(), '\n', readableBytes());
    if (pos == NULL)
    {
        return "";
    }
    //+1把\n也取出来
    return readAsString(pos - readPos() + 1);
}

/*

    Socket套接字模块

*/

class Socket
{
    static const int DEFAULT_BACKLOG = 64;

public:
    Socket() : _sockfd(-1) {}
    ~Socket()
    {
        Close();
    }
    Socket(int fd) : _sockfd(fd) {}

    bool Create()
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd < 0)
        {
            DF_FATAL("Socket create failed: %s", strerror(errno));
            return false;
        }

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
            DF_ERROR("ip error");
            return false;
        }

        if (bind(_sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        {
            DF_FATAL("Socket bind failed: %s", strerror(errno));
            return false;
        }
        return true;
    }

    bool Listen(int backlog = DEFAULT_BACKLOG)
    {
        if (listen(_sockfd, backlog) < 0)
        {
            DF_FATAL("Socket listen failed: %s", strerror(errno));
            return false;
        }
        return true;
    }

    //返回-2表示还可以重新accpet
    //返回-1表示accpet异常
    int Accept()
    {
        int fd = accept(_sockfd, NULL, NULL);
        if (fd < 0)
        {
            if(errno == EAGAIN || errno == EINTR) {
                return -2;
            }
            DF_ERROR("Accept new fd failed: %s", strerror(errno));
            return -1;
        }
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
        if (ret < 0)
        {
            DF_ERROR("Connect failed: %s", strerror(errno));
            return false;
        }
        return true;
    }

    void Close()
    {
        if (_sockfd >= 0)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }

    int Fd() const
    {
        return _sockfd;
    }

    // 接收数据（默认是阻塞接收）
    ssize_t Recv(void *buf, size_t len, int flags = 0)
    {
        assert(buf != nullptr);
        ssize_t ret = recv(_sockfd, buf, len, flags);

        if (ret <= 0)
        {
            //可以重新接收数据，返回0
            if(errno == EAGAIN || errno == EINTR){
                return 0;
            }
            //接收出错 or 连接断开，不能重新接收数据，返回-1
            DF_ERROR("Recv from fd-%d failed: %s", _sockfd, strerror(errno));
            return -1;
        }
        // 数据接收成功
        return ret;
    }
    // 非阻塞接收
    ssize_t NonBlockRecv(void *buf, size_t len)
    {
        return Recv(buf, len, MSG_DONTWAIT);
    }

    // 发送数据
    ssize_t Send(const void *buf, size_t len, int flags = 0)
    {
        assert(buf != nullptr);
        ssize_t ret = send(_sockfd, buf, len, flags);
        if (ret < 0)
        {
            //可以重新发送数据，返回0
            if(errno == EAGAIN || errno == EINTR){
                return 0;
            }
            //发送出错 or 连接断开，不能重新发送数据，返回-1
            DF_ERROR("Send toto fd-%d failed: %s", _sockfd, strerror(errno));
            return -1;
        }
        // 数据发送成功
        return ret;
    }
    ssize_t NonBlockSend(const void *buf, size_t len)
    {
        return Send(buf, len, MSG_DONTWAIT);
    }

    // 创建一个server连接
    bool CreateServer(const uint16_t &port, const std::string &ip = "0.0.0.0")
    {
        return Create() && SetNonBlock() && SetAddrReuse() && Bind(ip, port) && Listen();
    }

    // 创建一个client连接
    bool CreateClient(const std::string &ip, const uint16_t &port)
    {
        return Create() && Connect(ip, port);
    }

    // 设置套接字为地址重用
    bool SetAddrReuse()
    {
        int optval = 1;
        if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        {
            DF_ERROR("fd-%d SetAddrReuse", _sockfd);
            return false;
        }
        return true;
    }

    // 设置套接字为非阻塞
    bool SetNonBlock()
    {
        int flags = fcntl(_sockfd, F_GETFL, 0);
        if (fcntl(_sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            DF_ERROR("fd-%d SetNonBlock", _sockfd);
            return false;
        }
        return true;
    }

private:
    int _sockfd;
};
