#pragma once
/*Logger日志器统筹管理各个日志模块
    需要维护的成员
    1.日志器名称（唯一标识）
    2.日志限制等级（这个日志器只会落地限制等级及以上的日志）
    3.日志Formatter
    4.日志sinks（可以有多种落地方向）
    5.互斥锁（保护日志器在多线程访问下的安全）
*/

// Logger的两种类型：同步与异步 -> 类内类型标识字段区分

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <unordered_map>

#include <cstdarg>
#include <cassert>

#include "log_level.hpp"
#include "log_message.hpp"
#include "log_format.hpp"
#include "log_sinker.hpp"
#include "cirQueue.hpp"
#include "async_looper.hpp"
#include "buffer.hpp"

namespace ckflogs
{
    static const size_t BUFFER_SIZE = 1024;
    class Logger
    {
    public:
        using Ptr = std::shared_ptr<Logger>;
        enum LoggerType
        {
            LOGGER_SYNC,
            LOGGER_ASYNC
        };

    protected:
        std::string name_; // logger的唯一标识
        // LogLevel::Value limit_level_;         // 缺省是DEBUG
        Formatter formatter_;                      // 决定logger输出日志的格式
        std::vector<LogSinker::Ptr> sinkers_;      // 决定logger输出日志的方向
        std::atomic<LogLevel::Value> limit_level_; // 多线程使用日志器时，shouldLog函数频繁访问，设为原子类型保证线程安全即可，不用加锁
        std::mutex mutex_;                         // 多线程使用日志器时，保护日志器的线程安全

    public:
        Logger(const std::string &name,
               LogLevel::Value limit_level,
               const Formatter &formatter,
               const std::vector<LogSinker::Ptr> &sinkers)
            : name_(name),
              limit_level_(limit_level),
              formatter_(formatter),
              sinkers_(sinkers) {}

        virtual ~Logger() {}

        // 落地DEBUG等级的日志
        int debug(const std::string &filename, size_t line, const char *msg_fmt, ...)
        {
            // 0.判断DEBUG等级能否落地（限制等级）
            if (!shouldLog(LogLevel::Value::DEBUG))
                return -1;
            // 1.构建LogMessage对象，要依靠用户传递的文件名、行号和日志主体消息msg
            //  (1) 根据msg_fmt和vargs列表，生成日志主体消息msg
            va_list ap;
            va_start(ap, msg_fmt);
            char buf[BUFFER_SIZE] = {0};
            int n = vsnprintf(buf, sizeof(buf), msg_fmt, ap);
            if (n < 0)
                return -1;
            va_end(ap);
            buf[n] = 0;
            std::string msg = buf;
            size_t loglen = 0;
            log(LogLevel::Value::DEBUG, msg, filename, line, &loglen);
            return loglen;
        }

        int info(const std::string &filename, size_t line, const char *msg_fmt, ...)
        {
            if (!shouldLog(LogLevel::Value::INFO))
                return -1;
            va_list ap;
            va_start(ap, msg_fmt);
            char buf[BUFFER_SIZE] = {0};
            int n = vsnprintf(buf, sizeof(buf), msg_fmt, ap);
            if (n < 0)
                return -1;
            va_end(ap);
            buf[n] = 0;
            std::string msg = buf;
            size_t loglen = 0;
            log(LogLevel::Value::INFO, msg, filename, line, &loglen);
            return loglen;
        }

        int warn(const std::string &filename, size_t line, const char *msg_fmt, ...)
        {
            if (!shouldLog(LogLevel::Value::WARN))
                return -1;
            va_list ap;
            va_start(ap, msg_fmt);
            char buf[BUFFER_SIZE] = {0};
            int n = vsnprintf(buf, sizeof(buf), msg_fmt, ap);
            if (n < 0)
                return -1;
            va_end(ap);
            buf[n] = 0;
            std::string msg = buf;
            size_t loglen = 0;
            log(LogLevel::Value::WARN, msg, filename, line, &loglen);
            return loglen;
        }

        int error(const std::string &filename, size_t line, const char *msg_fmt, ...)
        {
            if (!shouldLog(LogLevel::Value::ERROR))
                return -1;
            va_list ap;
            va_start(ap, msg_fmt);
            char buf[BUFFER_SIZE] = {0};
            int n = vsnprintf(buf, sizeof(buf), msg_fmt, ap);
            if (n < 0)
                return -1;
            va_end(ap);
            buf[n] = 0;
            std::string msg = buf;
            size_t loglen = 0;
            log(LogLevel::Value::ERROR, msg, filename, line, &loglen);
            return loglen;
        }

        int fatal(const std::string &filename, size_t line, const char *msg_fmt, ...)
        {
            if (!shouldLog(LogLevel::Value::FATAL))
                return -1;
            va_list ap;
            va_start(ap, msg_fmt);
            char buf[BUFFER_SIZE] = {0};
            int n = vsnprintf(buf, sizeof(buf), msg_fmt, ap);
            if (n < 0)
                return -1;
            va_end(ap);
            buf[n] = 0;
            std::string msg = buf;

            // 以下三个动作都需要申请互斥锁mutex，因为都用到了Logger类内成员
            //  1.构建logmessage对象
            //  2.对logmessage对象进行序列化
            //  3.实际落地
            size_t loglen = 0;
            log(LogLevel::Value::FATAL, msg, filename, line, &loglen);
            return loglen;
        }

        template <class SinkerType, class... Args>
        void RegisterSinker(Args &&...args)
        {
            std::unique_lock<std::mutex> lck(mutex_);
            sinkers_.push_back(LogSinkerFactory::create<SinkerType>(std::forward<Args>(args)...));
        }

    protected:
        //  1.构建logmessage对象
        //  2.对logmessage对象进行序列化
        //  3.实际落地
        //  最后的参数rlen可获取这条落地日志的长度
        void log(LogLevel::Value level, const std::string &msg, const std::string &filename, size_t line, size_t *rlen = nullptr)
        {
            // 0.加锁保护本次日志落地的线程安全
            std::unique_lock<std::mutex> lck(mutex_);
            // 1.构建logmessage对象
            LogMessage logmsg(level, filename, line, name_, msg);
            // 2.对logmessage对象进行序列化
            std::string logstr = formatter_.format(logmsg);
            if (rlen != nullptr)
                *rlen = logstr.size();
            // 3.实际落地
            logSink(logstr);
        }
        // 同步和异步的实际落地方式不同
        virtual void logSink(const std::string &logstr) = 0;

        bool shouldLog(LogLevel::Value level)
        {
            return level >= limit_level_;
        }
    };
    /*同步日志器*/
    class SyncLogger : public Logger
    {
    public:
        SyncLogger(const std::string &name,
                   LogLevel::Value limit_level,
                   const Formatter &formatter,
                   const std::vector<LogSinker::Ptr> &sinkers)
            : Logger(name, limit_level, formatter, sinkers)
        {
        }

    private:
        void logSink(const std::string &logstr) override
        {
            assert(!sinkers_.empty());
            for (auto &sinker : sinkers_)
            {
                sinker->sink(logstr.c_str(), logstr.size());
            }
        }
    };

    class AsyncLogger : public Logger
    {

    public:
        AsyncLogger(const std::string &name,
                    LogLevel::Value limit_level,
                    const Formatter &formatter,
                    const std::vector<LogSinker::Ptr> &sinkers,
                    AsyncLooper::LooperType looper_type)
            : Logger(name, limit_level, formatter, sinkers),
              looper_(std::bind(&AsyncLogger::realLogSink, this, std::placeholders::_1), looper_type) {}

    private:
        void logSink(const std::string &logstr) override
        {
            // 将日志消息推送给异步工作器中, 由异步工作器完成日志实际落地
            looper_.push(logstr.c_str(), logstr.size());
        }

        void realLogSink(Buffer &buffer)
        {
            assert(!sinkers_.empty());
            for (auto &sinker : sinkers_)
            {
                sinker->sink(buffer.begin(), buffer.readAbleSize());
            }
        }

    private:
        AsyncLooper looper_;
    };

    /*
    缺省:
        类型：同步
        限制等级：DEBUG
        Formatter格式："[%d][%L][%t][%c][%f:%l]%T%m%n"
        落地方向：有且仅有-标准输出
    注意：日志器名称不能缺省，必须显式提供
    */
    class LoggerBuilder
    {
    public:
        using Ptr = std::shared_ptr<LoggerBuilder>;

    protected:
        std::string name_;
        Logger::LoggerType type_ = Logger::LoggerType::LOGGER_SYNC;
        LogLevel::Value limit_level_ = LogLevel::Value::DEBUG;
        Formatter formatter_;
        std::vector<LogSinker::Ptr> sinkers_;
        AsyncLooper::LooperType looper_type_ = AsyncLooper::LooperType::SAFE;

    public:
        LoggerBuilder() {}
        virtual ~LoggerBuilder() {}
        void bulidName(const std::string &name)
        {
            name_ = name;
        }
        void bulidType(Logger::LoggerType type)
        {
            type_ = type;
        }
        void bulidLevel(LogLevel::Value level)
        {
            limit_level_ = level;
        }
        void buildFormatter(const std::string &pattern)
        {
            formatter_ = Formatter(pattern);
        }
        void buildAsycnType(const std::string& flag)
        {
            assert(flag == "SAFE" || flag == "UNSAFE");
            if (flag == "SAFE")
                looper_type_ = AsyncLooper::LooperType::SAFE;
            else
                looper_type_ = AsyncLooper::LooperType::UNSAFE;
        }

        // 建造一个LogSink到sinks中
        template <class SinkerType, class... Args>
        void buildSinker(Args &&...args)
        {
            sinkers_.push_back(std::make_shared<SinkerType>(std::forward<Args>(args)...));
        }

        virtual Logger::Ptr build() = 0;
    };

    class LocalLoggerBuilder : public LoggerBuilder
    {
        Logger::Ptr build() override
        {
            assert(!name_.empty());
            if (sinkers_.empty())
                buildSinker<StdOutLogSinker>();
            if (type_ == Logger::LoggerType::LOGGER_SYNC)
                return std::make_shared<SyncLogger>(name_, limit_level_, formatter_, sinkers_);
            else // 异步Logger
                return std::make_shared<AsyncLogger>(name_, limit_level_, formatter_, sinkers_, looper_type_);
        }
    };

    /*
   管理全局的日志器logger, 便于在程序的任何位置使用
   - Member variables
       1.Default Logger (sink to stdout)
       2.Hash table (key: logger_name, value: Logger)
       3.Mutex (guarantee thread safe of LoggerManager)
   - Member function
       0.constructor (build default_logger and insert it into loggers)
       1.addLogger   (register new logger into LoggerManager::loggers_)
       2.existLogger (determine if a logger exist in LoggerManager by its name)
       3.findLogger  (find a logger by its name, and return the logger)
    */
    class LoggerManager
    {
    public:
        static LoggerManager &getInstance()
        {
            static LoggerManager instance;
            return instance;
        }

        void addLogger(const std::string &name, Logger::Ptr logger) // logger built by GlobalLoggerBuilder
        {
            std::unique_lock<std::mutex> lck(mutex_);
            loggers_[name] = logger;
        }

        bool existLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lck(mutex_);
            auto it = loggers_.find(name);
            return it != loggers_.end();
        }

        Logger::Ptr getLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lck(mutex_);
            auto it = loggers_.find(name);
            if (it == loggers_.end())
                return nullptr;
            return it->second;
        }

        Logger::Ptr defaultLogger()
        {
            return default_logger_;
        }

    private:
        // 设计为单例模式
        LoggerManager()
        {
            LoggerBuilder::Ptr builder = std::make_shared<LocalLoggerBuilder>();
            builder->bulidName("default_logger");
            default_logger_ = builder->build();
            loggers_["default_logger"] = default_logger_;
        }

        LoggerManager(const LoggerManager &lm) = delete;
        LoggerManager &operator=(const LoggerManager &lm) = delete;

    private:
        Logger::Ptr default_logger_;
        std::unordered_map<std::string, Logger::Ptr> loggers_;
        std::mutex mutex_;
    };

    class GlobalLoggerBuilder : public LoggerBuilder
    {
        Logger::Ptr build() override
        {
            assert(!name_.empty());
            if (sinkers_.empty())
                buildSinker<StdOutLogSinker>();

            Logger::Ptr logger;
            if (type_ == Logger::LoggerType::LOGGER_SYNC)
                logger = std::make_shared<SyncLogger>(name_, limit_level_, formatter_, sinkers_);
            else
                logger = std::make_shared<AsyncLogger>(name_, limit_level_, formatter_, sinkers_, looper_type_);

            LoggerManager::getInstance().addLogger(name_, logger);
            return nullptr;
        }
    };

    class LoggerBuilderFactory
    {
    public:
        template <class BuilderType>
        static LoggerBuilder::Ptr create()
        {
            return std::make_shared<BuilderType>();
        }
    };
} /*namespace ckflogs*/
