#pragma once
#include <iostream>
#include <string>
#include <thread>
#include "log_level.hpp"
#include "util.hpp"

/*
日志消息要包含的内容：
    1.时间
    2.日志等级
    3.文件名
    4.行号
    5.线程ID
    6.日志器名称
    7.日志消息主体
*/

namespace ckflogs
{
    struct LogMessage
    {
        time_t ctime_;          // 时间戳
        LogLevel::Value level_; // 日志等级
        std::string filename_;  // 文件名
        size_t line_;           // 行号
        std::thread::id tid_;   // 线程ID
        std::string logger_;    // 日志器名称
        std::string message_;   // 日志消息主体
        
        LogMessage(LogLevel::Value level, const std::string &filename, size_t line, const std::string &logger, const std::string &message)
            : ctime_(util::Date::now())
            , level_(level)
            , filename_(filename)
            , line_(line)
            , tid_(std::this_thread::get_id())
            , logger_(logger)
            , message_(message)
        {
        }
    };

}; /*namespace ckflogs*/