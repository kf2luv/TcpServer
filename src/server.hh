#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstring>

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
    //判断缓冲区是否为空
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