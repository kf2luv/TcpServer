#pragma once
#include <iostream>
#include "util.hpp"
#include "log_message.hpp"
#include "log_format.hpp"
#include "log_sinker.hpp"
#include "logger.hpp"

namespace ckflogs
{
    Logger::Ptr getLogger(const std::string &name)
    {
        return LoggerManager::getInstance().getLogger(name);
    }
    Logger::Ptr defaultLogger()
    {
        return LoggerManager::getInstance().defaultLogger();
    }

#define _debug(fmt, ...) debug(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define _info(fmt, ...) info(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define _warn(fmt, ...) warn(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define _error(fmt, ...) error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define _fatal(fmt, ...) fatal(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define DF_DEBUG(fmt, ...) ckflogs::defaultLogger()->_debug(fmt, ##__VA_ARGS__);
#define DF_INFO(fmt, ...) ckflogs::defaultLogger()->_info(fmt, ##__VA_ARGS__);
#define DF_WARN(fmt, ...) ckflogs::defaultLogger()->_warn(fmt, ##__VA_ARGS__);
#define DF_ERROR(fmt, ...) ckflogs::defaultLogger()->_error(fmt, ##__VA_ARGS__);
#define DF_FATAL(fmt, ...) ckflogs::defaultLogger()->_fatal(fmt, ##__VA_ARGS__);
}