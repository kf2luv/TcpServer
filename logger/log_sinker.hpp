#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <regex>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include "util.hpp"

namespace ckflogs
{
    // 日志落地方向
    // 1.stdout
    // 2.文件
    // 3.滚动文件

    // 抽象一个日志落地父类，然后不同的落地方向派生不同的子类

    class LogSinker
    {
    public:
        using Ptr = std::shared_ptr<LogSinker>;

    public:
        LogSinker() {}
        virtual ~LogSinker() {}
        virtual void sink(const char *log, size_t len) = 0;
    };

    class StdOutLogSinker : public LogSinker
    {
    public:
        void sink(const char *log, size_t len) override
        {
            // // 将输入的log字符串转化为std::string
            // std::string logStr(log, len);

            // // 使用正则表达式替换日志等级子串并添加颜色
            // logStr = replaceLogLevel(logStr, "DEBUG", "\033[34m[DEBUG]\033[0m");   // 蓝色
            // logStr = replaceLogLevel(logStr, "INFO", "\033[32m[INFO]\033[0m");     // 绿色
            // logStr = replaceLogLevel(logStr, "WARN", "\033[33m[WARN]\033[0m");     // 黄色
            // logStr = replaceLogLevel(logStr, "ERROR", "\033[31m[ERROR]\033[0m");   // 红色
            // logStr = replaceLogLevel(logStr, "FATAL", "\033[1;31m[FATAL]\033[0m"); // 粗体红色

            // // 输出日志消息
            // std::cout.write(logStr.c_str(), logStr.size());

            std::cout.write(log, len);
        }

    private:
    };

    class FileLogSinker : public LogSinker
    {
    private:
        std::string pathname_;
        std::ofstream ofs_;

    public:
        FileLogSinker(const std::string &pathname) : pathname_(pathname)
        {
            // 1.查看pathname的目录是否存在，不存在要创建
            std::string dir = util::File::getDirctory(pathname_);
            if (!util::File::exist(dir))
            {
                // 2.创建目录
                if (!util::File::createDirctory(dir))
                {
                    std::cout << "create dirctory fail..." << std::endl;
                    exit(1);
                }
            }
            // 3.打开文件
            ofs_.open(pathname_, std::ofstream::binary | std::ofstream::app);
            if (!ofs_.is_open())
            {
                std::cout << "日志文件打开失败" << std::endl;
            }
        }
        ~FileLogSinker()
        {
            if (ofs_.is_open())
                ofs_.close();
        }
        void sink(const char *log, size_t len) override
        {
            ofs_.write(log, len);
            ofs_.flush();

            if (!ofs_)
            {
                if (ofs_.fail())
                {
                    std::cerr << "Logical error on write (failbit set)." << std::endl;
                }
                if (ofs_.bad())
                {
                    std::cerr << "I/O error on write (badbit set)." << std::endl;
                }
            }
        }
    };

    // 滚动文件
    // 按文件大小滚动，每个文件最多存储1MB，超过1MB则关闭本文件并打开一个新文件
    static const size_t default_max_size = 1024 * 1024;
    class RollBySizeLogSinker : public LogSinker
    {
    private:
        std::string prefix_;    // 滚动文件名的前缀 （固定不变的）
        std::ofstream cur_ofs_; // 当前文件流（动态变化的）
        size_t cur_size_;       // 当前写入文件的字节数
        size_t max_size_;       // 滚动文件的临界字节数

    public:
        RollBySizeLogSinker(const std::string &prefix, size_t max_size = default_max_size)
            : prefix_(prefix), cur_size_(0), max_size_(max_size)
        {
            // 1.目录的创建
            std::string dir = util::File::getDirctory(prefix_);
            if (!util::File::exist(dir))
            {
                if (!util::File::createDirctory(dir))
                {
                    std::cout << "create dirctory fail..." << std::endl;
                    exit(1);
                }
            }
            // 2.创建文件
            createFile();
        }
        ~RollBySizeLogSinker()
        {
            if (cur_ofs_.is_open())
                cur_ofs_.close();
        }

        void sink(const char *log, size_t len) override
        {
            if (cur_size_ >= max_size_)
            {
                // 滚动到下一个文件
                if (cur_ofs_.is_open())
                {
                    cur_ofs_.close();
                    cur_size_ = 0;
                }
                createFile();
            }
            cur_ofs_.write(log, len);
            cur_size_ += len;
        }

    private:
        void createFile()
        {
            static int count = 0;
            // 1.在prefix的基础上，得到一个完整的文件名
            // filename: prefix + 202311291817550.log
            std::stringstream ss;
            ss << prefix_;
            util::Date d;
            ss << d.year << d.month << d.day << d.hour << d.min << d.sec;
            ss << "-" << count++;
            ss << ".log";
            std::string filename = ss.str();

            // 2.根据文件名打开文件(在得到的目录下创建并打开)
            cur_ofs_.open(filename, std::ofstream::binary | std::ofstream::app);
        }
    };

    // class MySQLSinker : public LogSinker
    // {
    // public:
    //     MySQLSinker(const std::string &table_name,
    //                 const std::string &host = "localhost",
    //                 const std::string &user = "mylog",
    //                 const std::string &passwd = "366836573",
    //                 const std::string &db = "log_db",
    //                 unsigned int port = 3369) : mysql_(nullptr), table_name_(table_name)
    //     {
    //         // 创建mysql句柄
    //         mysql_ = mysql_init(nullptr);

    //         // 连接mysql
    //         mysql_real_connect(mysql_,
    //                            host.c_str(),
    //                            user.c_str(),
    //                            passwd.c_str(),
    //                            db.c_str(),
    //                            port, nullptr, 0);
    //     }
    //     ~MySQLSinker()
    //     {
    //         // 关闭mysql
    //         mysql_close(mysql_);
    //     }

    //     void sink(const char *log, size_t len) override
    //     {
    //         std::string log_into_sql(log, len);
    //         if (log_into_sql[log_into_sql.size() - 1] == '\n')
    //         {
    //             log_into_sql.resize(log_into_sql.size() - 1);
    //         }
    //         std::string query = "insert into " + table_name_ + " values " + "('" + log_into_sql + "')";
    //         mysql_query(mysql_, query.c_str());
    //     }

    // private:
    //     MYSQL *mysql_;
    //     std::string table_name_;
    // };

    class LogSinkerFactory
    {
    public:
        template <class LogSinkerType, class... Args>
        static LogSinker::Ptr create(Args &&...args)
        {
            return std::make_shared<LogSinkerType>(std::forward<Args>(args)...);
        }
    };

}; /*namespace ckflogs*/