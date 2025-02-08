# 1. 目标实现

> **muduo库** 是一个基于 C++ 的高性能网络库，在github开源，专注于高并发网络服务的开发。它采用了事件驱动的非阻塞 I/O 模型（Reactor 模式），为开发高性能服务器应用程序提供了高效、稳定和易用的解决方案。本项目抱着学习的心态来开发，以达到加深对Reactor模型的理解，熟悉高性能服务器“轮子”框架的目的，为以后在应用层业务开发中打下基础。

[muduo github项目地址](https://github.com/chenshuo/muduo)

本项目仿照muduo库实现一个基于**One Thread One Loop主从Reactor模型**的轻量级高性能并发服务器，结构如下：

1. **主Reactor**

   负责连接管理，主要工作是：监听客户端的连接请求，收到新连接后，向Reactor分发连接

   ![image-20250119005646815](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202501190056897.png)

2. **从Reactor**

   负责IO事件监听、IO操作和业务处理。因为我目前的条件只能在单主机部署项目，因此没有引入工作线程来处理业务，以此减少并发执行流，减轻CPU切换调度的负担。

   ![image-20250119005723020](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202501190057056.png)



# 2. 模块划分 



**各模块功能以及相互联系的理解 TODO**

`Acceptor`监听连接管理，获取新连接，初始化，创建`Connection`对象，并为其设置一系列回调函数。`Connection`描述符的事件由`Channel`管理，每个事件的发生设有一个回调函数。

连接建立完成后，要启用对该连接的事件监控，`Connection`对应的`Channel`去找`EventLoop`注册事件监控，并添加非热点连接销毁任务（定时任务）。

一个线程对应一个`EventLoop`，一个`EventLoop`可以管理多个`Connection`。`EventLoop`里还有一个任务队列，用以支持事件的延迟任务执行和跨线程任务调度（对于当前`EventLoop`管理的连接，只能在当前线程中操作）。

`EventLoop`发现`Channel`的事件触发（如读事件），`Channel`调用对应的回调函数，将所属`Connection`内部`Socket`的数据读取到内部`Buffer`读缓冲区上，由于读缓冲区有数据了，此时又触发了`TcpServer`给`Connection`设置的新数据接收后的回调，再往上就是对数据的业务处理了。





# 3. 基础功能

## **定时器**

时间轮思想，用一个数组和一个tick指针实现定时器的功能。数组中存放定时任务，tick指针每秒走一步，走到哪个任务，就执行哪个任务。数组中每个位置存放的是多个任务（任务数组），因为同一时间可能要多个定时任务要被执行。

![image-20250123020829580](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202501230208610.png)

为了实现定时任务的超时时间刷新，需要用到两个技术：类的析构函数和`shared_ptr`。我们假设将定时任务封装为一个类，存储在时间轮中，而定时任务的处理在类对象析构时执行。定时任务超时时间重置后，就会在时间轮中存在前后两个待处理的相同任务，在我们的意料中，前一个任务是不需要执行的（因为已经重置了），那有什么方法让其不执行，也就是定时任务类不析构呢？`shared_ptr`可以解决这个问题：如果在时间轮中存储的不是定时任务`TimerTask`，而是指向`TimerTask`的智能指针`shared_ptr<TimerTask>`，对于指向相同`TimerTask`的多个`shared_ptr<TimerTask>`，因为**计数器机制**，只会在最后一个（也就是时间轮中最新的、需要执行的任务）引用处析构`TImerTask`，执行定时工作任务。

![image-20250123020812010](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202501230208058.png)

## Any类的设计思想

Any类是一个可以接收并保存任意类型数据的容器，并且保存的类型可以动态变化，即收到什么就保存什么。内部用到了一个多态的父类指针`Holder`作为数据的引用，在Any构造时根据传入的参数类型，构造子类对象`PlaceHolder`。



<img src="https://ckfs.oss-cn-beijing.aliyuncs.com/img/202501240037597.png" alt="image-20250124003702500" style="zoom:50%;" />



# 4. 模块实现



## Buffer

 数据缓冲区，提供数据写入和读取功能。

![image-20250124220940375](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202501242209468.png)

`read_idx`为读偏移，用户每次从这里开始读取数据

`write_idx`为写偏移，用户每次从这里开始写入数据

- **写入操作：**

  确保空间充足后（挪动数据 或 扩容），再去写入数据。

  

- **读取操作：**

  从`read_idx`开始读取数据，如果可读数据长度小于待读取长度，则有多少读多少。



由于不确定用户传入的数据类型，我们可以定义缓冲区以字节为单位，设为`std::vector<char>`方便空间管理。



## Socket

套接字模块，就是简单把socket操作的几个系统调用封装一下，方便使用。



## Channel

事件管理器，用于对一个描述符所关心的事件进行管理，设置对应事件触发的回调函数，共可以支持以下五种事件：

1. 可读事件
2. 可写事件
3. 错误事件
4. 连接断开事件
5. 任意事件

`Channel`只负责管理描述符关心的事件，以及对事件触发后的处理逻辑做管理，真正监控事件是否发生由`EventLoop`负责。



## Poller

事件监控器，描述符IO事件监控模块，底层使用`epoll`。相当于对`epoll`进行封装，方便监控操作。

`Poller`的封装思想：

1. 一个`epoll`句柄；
2. 一个`struct epoll_event`数组`events`，用于保存`epoll_wait`返回后，所有内核监控到的活跃事件；
3. 一个`hash`表，管理描述符与其对应的事件管理器`Channel`的映射关系。



`Poller`提供的功能

1. 添加or更新**描述符**及其所监控的**事件**。（`Channel`中集成了这一组信息）；
2. 移除描述符的监控； 
3. 开始监控

`Channel`与`Poller`的联调测试

![image-20250128031422964](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202501280314058.png)





## EventLoop

**事件循环**，主要负责以下工作：

1.  **事件监控**
2.  **事件处理**
3.  **任务执行**

`EventLoop`是`One Thread One Loop`的设计核心，一个线程绑定一个`EventLoop`，一个连接的所有事件都在一个线程中监控和处理，由与线程绑定的`EventLoop`管理。这种设计简化了并发编程，因为不需要担心多线程环境下的竞态条件。`EventLoop` 不仅处理 I/O 事件，还处理**定时器事件**和**用户任务**。定时器用于在指定的时间点执行某些操作，而任务队列用于在事件循环中执行用户定义的任务。

**任务队列**

任务队列实现**跨线程调度**，`EventLoop`的“任务执行”指的就是执行任务队列中的任务。在多线程模型中，其他线程如果需要在 `EventLoop` 所在线程中执行某些操作（例如更新连接状态、发送数据等），会将任务放入任务队列，由 `EventLoop` 在合适时机执行。这样既保证了线程安全，又避免了直接跨线程操作带来的复杂性。

**定时器**

将时间轮定时器`TimerWheel`与内核`timerfd`整合到一起，`timerfd`负责超时事件的通知，`TimerWheel`负责执行每次到期的所有定时任务，形成一个真正的秒级定时器。`timerfd`的事件监控，也注册到`EventLoop`中。

对于当前`EventLoop`（或者说当前线程）的定时器，可能会被其它线程访问（如向当前定时器添加定时任务），此时就要考虑线程安全问题。我们考虑这样的做法：对于定时器的各种操作，不直接执行，而是放到任务队列中统一由拥有此定时器的线程执行，避免了多线程竞争问题。



`EventLoop`与各个模块整合为简单`TcpServer`关系图

![image-20250205234447550](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202502052344747.png)

 



## Connection

`Connection`**是连接管理模块，对一个连接进行全方位的管理：**

1. 套接字的管理 `Socket`
2. 连接的事件管理 `Channel`
3. 输入缓冲区和输出缓冲区的管理 `Buffer`
4. 协议上下文的管理 `Any`
5. 用户回调函数的管理

用户回调函数设定连接在不同状态下需要处理的任务，如：群聊服务器中，用户上线时（连接建立成功），向其他用户群发该用户上线通知。

`Connection`**提供的功能：**

1. 发送数据（实际是先将数据拷贝到发送缓冲区中，并开启写事件监控，等到写事件就绪时再向套接字写入数据）
2. 接收数据
3. 设置回调函数
4. 关闭连接
5. 启动非活跃连接超时断开
6. 取消非活跃连接超时断开

![image-20250207140426242](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202502071404307.png)



## Acceptor

对**监听套接字**进行管理，功能如下：

1. 创建一个监听套接字`ListenSocket`
2. 启动读事件监控，开始监听
3. 读事件触发，获取到新连接
4. 调用上层设置的回调函数，对新连接进行处理



## LoopThread

事件循环线程，`EventLoop`的工作线程，一个`EventLoop`工作在一个`LoopThread`中。

`LoopThread`的功能：

1. 在线程内实例化一个`EventLoop`，保证每个`EventLoop`实例化时就绑定一个唯一的线程ID
2. 启动`EventLoop`
3. 提供一个外部获取`EventLoop`的接口，

`LoopThread`对象一旦实例化，其内部便创建并启动了一个**事件循环**线程，并为外部提供了一个获取内部`EventLoop`的接口。`LoopThread`是外部操作这个线程的句柄。



## LoopThreadPool

事件循环线程池，用于对`LoopThread`进行管理和操作，功能如下：

1. **线程数量可配置**，支持0个或多个`LoopThread`。如果是0个，代表没有**从属线程**（单Reactor模型），连接获取和业务处理都在**主线程**的`EventLoop`中进行。
2. **管理**创建出来的`LoopThread`及其`EventLoop`，并维护一个主线程的`EventLoop`，以支持单Reactor模型。
3. **线程分配功能**：多Reactor模型中，当主线程接收到一个新连接，需要将新连接挂载到某个从属线程的`EventLoop`上，进行事件监控与处理，由`LoopThreadPool`分配这个从属线程（这里先采用简单的**RR轮转**策略分配线程，后面考虑优化）。单Reactor模型中，新连接直接由主线程的`EventLoop`处理。



## TcpServer

`TcpServer`整合模块，对所有功能子模块进行封装，其组成如下：

1. 自增长的连接ID，为每一个新连接分配一个唯一的ID
2. 一个监听管理套接字管理`accpetor`
3. 主线程事件循环`base_loop`，主要用于监听事件的监控
4. `loop_thread_pool`从线程池，管理所有的从属线程
5. 管理所有的连接`conn_map`
6. 各种回调函数（消息回调，连接建立回调，连接关闭回调，任意事件回调），由上层设置

功能：

1. 设置管理的从属线程数量
2. 设置各种回调函数（消息业务处理回调，连接建立回调，连接关闭回调，任意事件回调）
3. 开启空闲连接超时关闭
4. 添加一个定时任务（主线程事件循环中执行）
5. 启动服务器

`TcpServer`逻辑框图

![image-20250208023506491](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202502080235553.png)



*Bug Record1：*

![image-20250208013752754](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202502080137821.png)

> epoll错误，fd=3是刚开始分配的第一个描述符，所以去查看初始化服务器时，需要分配描述符的操作，发现TcpServer中代码出错，fd=3是acceptor的描述符，而acceptor在base_looper之前创建，无法找到挂载的EventLoop，因此出错。解决方法：在TcpServer的初始化列表中，调转acceptor和base_looper的顺序



*Bug Record2*

使用webbench对基于TcpServer设计的EchoServer进行压力测试，发现**webbench测试时间一到，客户端主动与server断开连接后**，server并不能正确地关闭连接，而是报如下错误：

![image-20250209014932864](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202502090149002.png)

此时系统的CPU占用率如下，可见被server进程消耗了非常多资源，但又没有客户端连接，浪费在哪？

![image-20250209015133859](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202502090151921.png)

排查与解决思路：

![image-20250209020303342](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202502090203423.png)

**鉴定为对连接挂断时，缓冲区尚存数据的处理不当，导致服务端一直没有关闭已经无效的连接，而且读事件（对端关闭）一直通知（epoll水平模式），服务端一直做无效的处理，大大浪费CPU资源，合理的做法应该是：**

- 客户端主动断开连接，in_buffer还有数据的话就先处理，out_buffer还有数据就丢弃（因为发不出去了）
- 服务端主动断开连接，in_buffer还有数据的话就先处理，out_buffer还有数据就先发送

因此，在`handleRead`中读取数据时发现是对端关闭了连接，就应该以看看`in_buffer`中还有没有数据，有的话处理，然后就直接关闭连接，不用管`out_buffer`（这是`handleClose`的逻辑，可直接调用）

```cpp
// 描述符可读事件发生
void handleRead()
{
    // 1.把socket中的数据读到in_buffer中
    char buf[65536]{};
    ssize_t ret = _socket.NonBlockRecv(buf, sizeof(buf));
    if (ret < 0)
    {
        DF_DEBUG("连接fd: %d 被挂断了, 尝试关闭连接", _socket.Fd());
        // 读取失败，关闭连接
        // shutdown();
        handleClose();
        return;
    }
    _in_buffer.write(buf, ret);

    // 2.调用业务处理回调函数
    if (_in_buffer.readableBytes() > 0)
    {
        _message_cb(shared_from_this(), _in_buffer);
    }
}
// 描述符关闭事件发生
void handleClose()
{
    // 看看读缓冲区有没有需要处理的数据，然后再关闭
    if (_in_buffer.readableBytes() > 0)
    {
        if (_message_cb)
        {
            _message_cb(shared_from_this(), _in_buffer);
        }
    }
    release();
}
```

修改后，可观察到此时客户端断连后，不再有日志的ERROR报错，htop查询到CPU使用率也恢复正常，说明连接被正常关闭

![image-20250209021211307](https://ckfs.oss-cn-beijing.aliyuncs.com/img/202502090212414.png)

## HTTP协议模块

