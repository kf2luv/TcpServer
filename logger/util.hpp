// 实用类设计
// 1.获取系统当前时间
// 2.创建目录（判断文件是否存在->获取文件的所在目录路径）

// 形如./abc/def/ghi的文件路径，当它不存在时，系统调用不能帮我们直接创建
// 因此日志落地到某个特定文件路径时，需要我们自己先创建目录

#pragma once

#include <iostream>
#include <string>
#include <ctime>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ckflogs
{
    namespace util
    {
        /*创建Date对象时，构造函数会构建当前时间的年月日时分秒在对象中*/
        class Date
        {
        public:
            Date(time_t time = now())
            {
                get(time);
            }
            static time_t now()
            {
                return time(nullptr);
            }
            static void now(time_t *t)
            {
                time(t);
            }
            void get(time_t time) // 根据C时间戳，更新当前时间
            {
                struct tm *t = localtime(&time);
                year = t->tm_year + 1900;
                month = t->tm_mon + 1;
                day = t->tm_mday;
                hour = t->tm_hour;
                min = t->tm_min;
                sec = t->tm_sec;
            }

        public:
            int year;
            int month;
            int day;
            int hour;
            int min;
            int sec;
        };

        class File
        {
        public:
            static bool exist(const std::string &pathname) // 判断文件是否存在
            {
                struct stat buf;
                return stat(pathname.c_str(), &buf) == 0;
            }

            // ./abc/def/ghi
            // 获取目录路径，然后创建该目录
            // 目录有了，文件可以由系统调用自动创建

            static std::string getDirctory(const std::string &pathname)
            {
                // pathname =  "./abc/def/ghi/jkl"
                size_t pos = pathname.find_last_of("/\\"); // 支持Linux和Windows双系统
                if (pos == std::string::npos)              // 没有找到'/'，那目录就是当前目录
                    return ".";
                return pathname.substr(0, pos + 1); // 加上了目录最后的'/'
            }

            static bool createDirctory(const std::string &dir)
            {
                // dir = "./abc/def/ghi/" (最后的'/'加不加都可以)
                // mkdir系统调用，创建./abc/def/ghi/，ghi之前的目录必须存在
                // 因此先创建abc，再创建abc/def，以此类推
                if (dir.empty())
                    return false;
                if (File::exist(dir))
                    return true;
                int index = 0;
                mode_t dir_mode = 0777;

                while (index < dir.size())
                {
                    size_t pos = dir.find_first_of("/\\", index);
                    if (pos == std::string::npos)
                        pos = dir.size();

                    std::string cur_dir = dir.substr(0, pos);
                    index = pos + 1;

                    // std::cout << "cur_dir: " << cur_dir << std::endl;

                    if (File::exist(cur_dir))
                        continue;

                    int ret = mkdir(cur_dir.c_str(), dir_mode);
                    if (ret < 0)
                        return false;
                }
                return true;
            }
        };

    }; // namespace util

} // namespace ckflogs
