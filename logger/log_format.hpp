#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <ctime>
#include <cassert>
#include "util.hpp"
#include "log_message.hpp"

/*
    用户自定义日志消息的格式化模式
    如：[%d{%H:%m:%S}][%L][%f:%l]%T[%m]%n
    格式化子项表示如下：
    %d: 时间    %Y%M%D%H%m%S: 时间的子格式，分别表示年月日时分秒
    %L: 日志等级
    %f: 文件名
    %l: 行号
    %t: 线程ID
    %c: logger
    %m: 日志消息主体
    %T: 缩进
    %n: 换行
*/
namespace ckflogs
{
    // 每一个格式子项都有一个类，这个类的作用是将子项格式对应的日志消息内容输出
    class FormatItem
    {
    public:
        using Ptr = std::shared_ptr<FormatItem>;

    public:
        virtual void outputItem(std::ostream &out, LogMessage msg) = 0;
        ~FormatItem() {}
    };

    class DateFormatItem : public FormatItem
    {
    private:
        std::string date_fmt_;

    public:
        DateFormatItem(const std::string &date_fmt = "%H:%m:%S") : date_fmt_(date_fmt) {}

        void outputItem(std::ostream &out, LogMessage msg)
        {
            std::string date_str;
            if (!dateToString(&date_str, msg.ctime_))
            {
                std::cout << "时间子项创建失败" << std::endl;
                return;
            }
            out << date_str;
        }

        bool dateToString(std::string *ret, time_t ctime);
    };

    class LevelFormatItem : public FormatItem
    {
    public:
        void outputItem(std::ostream &out, LogMessage msg)
        {
            out << LogLevel::toString(msg.level_);
        }
    };

    class FilenameFormatItem : public FormatItem
    {
    public:
        void outputItem(std::ostream &out, LogMessage msg)
        {
            out << msg.filename_;
        }
    };

    class LineFormatItem : public FormatItem
    {
    public:
        void outputItem(std::ostream &out, LogMessage msg)
        {
            out << msg.line_;
        }
    };

    class ThreadIDFormatItem : public FormatItem
    {
    public:
        void outputItem(std::ostream &out, LogMessage msg)
        {
            out << msg.tid_;
        }
    };

    class LoggerFormatItem : public FormatItem
    {
    public:
        void outputItem(std::ostream &out, LogMessage msg)
        {
            out << msg.logger_;
        }
    };

    class MessageFormatItem : public FormatItem
    {
    public:
        void outputItem(std::ostream &out, LogMessage msg)
        {
            out << msg.message_;
        }
    };

    class TabFormatItem : public FormatItem
    {
    public:
        void outputItem(std::ostream &out, LogMessage msg)
        {
            out << '\t';
        }
    };

    class NFormatItem : public FormatItem
    {
    public:
        void outputItem(std::ostream &out, LogMessage msg)
        {
            out << '\n';
        }
    };

    class OtherFormatItem : public FormatItem
    {
    private:
        std::string other_;

    public:
        OtherFormatItem(const std::string &other) : other_(other) {}
        void outputItem(std::ostream &out, LogMessage msg)
        {
            out << other_;
        }
    };

    class FormatItemFactory
    {
    public:
        using Ptr = std::shared_ptr<FormatItemFactory>;

    public:
        virtual FormatItem::Ptr Create(const std::string &val) = 0;
    };

    class DateFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            if (!val.empty())
                return std::make_shared<DateFormatItem>(val);
            else
                return std::make_shared<DateFormatItem>();
        }
    };

    class LevelFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            return std::make_shared<LevelFormatItem>();
        }
    };

    class FilenameFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            return std::make_shared<FilenameFormatItem>();
        }
    };

    class LineFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            return std::make_shared<LineFormatItem>();
        }
    };

    class ThreadIDFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            return std::make_shared<ThreadIDFormatItem>();
        }
    };

    class LoggerFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            return std::make_shared<LoggerFormatItem>();
        }
    };

    class MessageFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            return std::make_shared<MessageFormatItem>();
        }
    };

    class TabFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            return std::make_shared<TabFormatItem>();
        }
    };

    class NFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            return std::make_shared<NFormatItem>();
        }
    };

    class OtherFormatItemFactory : public FormatItemFactory
    {
    public:
        FormatItem::Ptr Create(const std::string &val)
        {
            return std::make_shared<OtherFormatItem>(val);
        }
    };

    FormatItem::Ptr CreateItem(FormatItemFactory::Ptr factory, const std::string &val = "")
    {
        return factory->Create(val);
    }

    /*根据指定格式pattern, 将日志消息msg格式化为一条字符串(可选:并将其输出到ostream流中) */
    class Formatter
    {
        static const std::string default_pattern;

    private:
        std::string pattern_;                // 格式化模式
        std::vector<FormatItem::Ptr> items_; // 由pattern拆分出的子项item序列

    public:
        Formatter(const std::string &pattern = default_pattern) : pattern_(pattern) {}

        void formatTo(std::ostream &out, const LogMessage &msg)
        {
            // 1.先对pattern作拆分，区分每一个格式化子项，并将这些子项按顺序存储
            if (!ParsePattern())
            {
                std::cout << "格式规则pattern分析出错" << std::endl;
                abort();
            }
            // 2.获得一个有序的子项格式序列，依次将子项格式对应的日志消息内容输出
            for (auto &item : items_)
            {
                item->outputItem(out, msg);
            }
            // 3.对items_进行清空
            items_.clear();
        }

        std::string format(const LogMessage &msg)
        {
            std::ostringstream oss;
            formatTo(oss, msg);
            return oss.str();
        }

    private:
        bool ParsePattern()
        {
            // [%%%%d{%H:%m:%S}][%L][%f:%l][%m]%n
            std::string value;
            bool in_brace = false;
            for (int i = 0; i < pattern_.size(); i++)
            {
                if (pattern_[i] != '%' || in_brace) // 如果检测到当前i指向在{}里面，可以直接跳过，{}里的内容不应该在此处理
                {
                    if (pattern_[i] == '{')
                        in_brace = true;
                    if (pattern_[i] == '}')
                        in_brace = false;
                    value += pattern_[i];
                    continue;
                }

                if (++i >= pattern_.size())
                {
                    std::cout << "非法日志格式: %号后缺少格式控制符" << std::endl;
                    return false;
                }
                // 此时i指向紧随%之后的字符
                if (pattern_[i] == '%') // 就是要输出%号, 那就是%%->%, value保留一个%即可
                {
                    value += pattern_[i];
                    continue;
                }

                // 走到这代表真正要进行格式控制了
                // 此时pattern_[i==keyi]是某一个格式化key，如'd'
                // 先将前面的非格式化字符串保存
                items_.push_back(CreateItemByKey('o', value));
                value.clear();

                // 查看格式标识符后面有没有子格式{}, 有的话保存到value
                int keyi = i;
                if (keyi + 1 < pattern_.size() && pattern_[keyi + 1] == '{')
                {
                    // 找'}'
                    int begin = keyi + 2, end = begin;
                    while (end < pattern_.size() && pattern_[end] != '}')
                        end++;
                    if (end >= pattern_.size()) // 没找到'}'
                    {
                        std::cout << "非法日志格式: 没有匹配的'}'" << std::endl;
                        return false;
                    }
                    // 找到了'}'
                    value = pattern_.substr(begin, end - begin);
                    i = end;
                }

                FormatItem::Ptr item = CreateItemByKey(pattern_[keyi], value);
                if (item == nullptr)
                {
                    std::cout << "非法日志格式: 格式控制符不存在: %" << pattern_[keyi] << std::endl;
                    return false;
                }
                items_.push_back(item);
                if (!value.empty()) // 经过了子格式的设置, value才不为空
                    value.clear();
            }
            return true;
        }

        FormatItem::Ptr CreateItemByKey(const char &key, const std::string &value)
        {
            switch (key)
            {
            case 'd':
                return CreateItem(std::make_shared<DateFormatItemFactory>(), value);
            case 'L':
                return CreateItem(std::make_shared<LevelFormatItemFactory>());
            case 'f':
                return CreateItem(std::make_shared<FilenameFormatItemFactory>());
            case 'l':
                return CreateItem(std::make_shared<LineFormatItemFactory>());
            case 't':
                return CreateItem(std::make_shared<ThreadIDFormatItemFactory>());
            case 'c':
                return CreateItem(std::make_shared<LoggerFormatItemFactory>());
            case 'm':
                return CreateItem(std::make_shared<MessageFormatItemFactory>());
            case 'T':
                return CreateItem(std::make_shared<TabFormatItemFactory>());
            case 'n':
                return CreateItem(std::make_shared<NFormatItemFactory>());
            case 'o':
                return CreateItem(std::make_shared<OtherFormatItemFactory>(), value);
            }
            return nullptr;
        }
    };
    const std::string Formatter::default_pattern = "[%d][%L][%t][%c][%f:%l]%T%m%n";
}; /*namespace ckflogs*/

/*definition*/
bool ckflogs::DateFormatItem::dateToString(std::string *ret, time_t ctime)
{
    util::Date date(ctime);
    // %H:%m:%S
    for (int i = 0; i < date_fmt_.size(); i++)
    {
        if (date_fmt_[i] != '%')
        {
            *ret += date_fmt_[i];
            continue;
        }
        // date_fmt_[i] == '%'
        if (i + 1 < date_fmt_.size())
        {
            switch (date_fmt_[++i])
            {
            case 'Y':
                *ret += std::to_string(date.year);
                break;
            case 'M':
                *ret += std::to_string(date.month);
                break;
            case 'D':
                *ret += std::to_string(date.day);
                break;
            case 'H':
                *ret += std::to_string(date.hour);
                break;
            case 'm':
                *ret += std::to_string(date.min);
                break;
            case 'S':
                *ret += std::to_string(date.sec);
                break;
            default:
            {
                std::cout << "非法时间子格式" << std::endl;
                return false;
            }
            }
        }
        else
        {
            std::cout << "%后缺少时间格式控制符" << std::endl;
            return false;
        }
    }
    return true;
}

// ckflogs::FormatItem::Ptr ckflogs::CreateItem(int key, std::string val)
// {
//     switch (key)
//     {
//     case 'd':
//         return std::make_shared<DateFormatItem>(val);
//     case 'L':
//         return std::make_shared<LevelFormatItem>();
//     case 'f':
//         return std::make_shared<FilenameFormatItem>();
//     case 'l':
//         return std::make_shared<LineFormatItem>();
//     case 't':
//         return std::make_shared<ThreadIDFormatItem>();
//     case 'c':
//         return std::make_shared<LoggerFormatItem>();
//     case 'm':
//         return std::make_shared<MessageFormatItem>();
//     case 'T':
//         return std::make_shared<TabFormatItem>();
//     case 'n':
//         return std::make_shared<NFormatItem>();
//     }
//     return std::make_shared<OtherFormatItem>(val);
// }
