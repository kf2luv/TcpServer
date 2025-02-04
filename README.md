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
