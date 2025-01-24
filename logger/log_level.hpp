#pragma once
#include <iostream>
#include <string>

namespace ckflogs
{
    class LogLevel
    {
    public:
        enum Value
        {
            UNKNOW = 0,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            OFF
        };

        static std::string toString(const Value &level)
        {
            switch (level)
            {
            case DEBUG:
                return "DEBUG";
            case INFO:
                return "INFO";
            case WARN:
                return "WARN";
            case ERROR:
                return "ERROR";
            case FATAL:
                return "FATAL";
            case OFF:
                return "OFF";
            }
            return "UNKNOW";
        }
    };
}; /*namespace ckflogs*/
