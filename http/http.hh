#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include "../src/server.hh"

#include "../logger/ckflog.hpp"

class Util 
{
public:
    // 分割字符串，以sep作为分隔字符串，对str进行分割，最终结果保存到result中，返回分割的次数
    static size_t split(const std::string &str, const std::string &sep, std::vector<std::string> &result)
    {
        size_t begin = 0, end = 0;
        while (begin < str.size() && end < str.size())
        {
            end = str.find_first_of(sep, begin);
            if (end == std::string::npos)
            {
                end = str.size();
            }
            std::string sub_str = str.substr(begin, end - begin);
            if (!sub_str.empty())//空串不加入结果
            {
                result.push_back(sub_str);
            }
            begin = end + sep.size();
        }
        return result.size();
    }

    // 读取文件内容
    static bool readFile(const std::string &file_path, std::string& data)
    {
        // 1.打开文件
        std::ifstream ifs(file_path.c_str(), std::ios::binary);
        if (!ifs.is_open())
        {
            DF_ERROR("Open file %s failed", file_path.c_str());
            return false;
        }
        // 2.获取文件大小
        ifs.seekg(0, ifs.end);
        size_t fsize = ifs.tellg();
        ifs.seekg(0, ifs.beg);
        
        // 3.从ifs读取文件全部内容，写入buffer中
        data.resize(fsize);
        ifs.read(&data[0], fsize);
        if(!ifs.good())
        {
            DF_ERROR("Read file %s failed", file_path.c_str());
            return false;
        }

        // 3.关闭文件
        ifs.close();
        return true;
    }

    // 写入文件内容
    static bool writeFile(const std::string &file_path, const std::string& data)
    {
        // 1.打开文件
        std::ofstream ofs(file_path.c_str(), std::ios::binary);
        if (!ofs.is_open())
        {
            DF_ERROR("Open file %s failed", file_path.c_str());
            return false;
        }
        // 2.向ofs写入文件内容
        ofs.write(data.c_str(), data.size());
        if(!ofs.good())
        {
            DF_ERROR("Write file %s failed", file_path.c_str());
            return false;
        }

        // 3.关闭文件
        ofs.close();
        return true;
    }

    // URL编码
    static std::string urlEncode(const std::string url, bool convertSpace = false)
    {
        std::string result;
        for(auto& c : url)
        {
            if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') //普通数字和字母字符，还有非转换符号
            {
                result += c;
            }
            else if(c == ' ' &&  convertSpace)
            {
                result += '+'; 
            }
            else
            {
                char tmp[4]{};
                snprintf(tmp, sizeof(tmp), "%%%02X", c);
                result += tmp;
            }
        }
        return result;
    }

    // URL解码
    static std::string urlDecode(const std::string url, bool convertPlus = false)
    {
        //遇到%就解码(将紧随其后的两个数字转换为字符)，遇到+就替换为空格
        std::string result;
        size_t i = 0;
        while(i < url.size())
        {            
            if(url[i] == '+' && convertPlus)
            {
                result += ' ';
                i++;
                continue;
            }

            if(url[i] == '%')
            {
                assert(i + 2 < url.size()); //至少还有两个字符
                char c = (char)std::stoi(url.substr(i+1, 2), nullptr, 16);
                result += c;
                i += 3;
            }
            else
            {
                result += url[i];
                i++;
            }
        }
        return result;
    }

    // 获取响应状态码对应的描述信息
    static std::string getStatusDesc(int stat_code)
    {
        switch (stat_code)
        {
            // 1xx Informational
            case 100: return "Continue";
            case 101: return "Switching Protocols";
            case 102: return "Processing";
            case 103: return "Early Hints";
    
            // 2xx Success
            case 200: return "OK";
            case 201: return "Created";
            case 202: return "Accepted";
            case 203: return "Non-Authoritative Information";
            case 204: return "No Content";
            case 205: return "Reset Content";
            case 206: return "Partial Content";
            case 207: return "Multi-Status";
            case 208: return "Already Reported";
            case 226: return "IM Used";
    
            // 3xx Redirection
            case 300: return "Multiple Choices";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 303: return "See Other";
            case 304: return "Not Modified";
            case 305: return "Use Proxy";
            // 306 已经被弃用
            case 307: return "Temporary Redirect";
            case 308: return "Permanent Redirect";
    
            // 4xx Client Error
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 402: return "Payment Required";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 406: return "Not Acceptable";
            case 407: return "Proxy Authentication Required";
            case 408: return "Request Timeout";
            case 409: return "Conflict";
            case 410: return "Gone";
            case 411: return "Length Required";
            case 412: return "Precondition Failed";
            case 413: return "Payload Too Large";
            case 414: return "URI Too Long";
            case 415: return "Unsupported Media Type";
            case 416: return "Range Not Satisfiable";
            case 417: return "Expectation Failed";
            case 418: return "I'm a teapot"; // 彩蛋
            case 421: return "Misdirected Request";
            case 422: return "Unprocessable Entity";
            case 423: return "Locked";
            case 424: return "Failed Dependency";
            case 425: return "Too Early";
            case 426: return "Upgrade Required";
            case 428: return "Precondition Required";
            case 429: return "Too Many Requests";
            case 431: return "Request Header Fields Too Large";
            case 451: return "Unavailable For Legal Reasons";
    
            // 5xx Server Error
            case 500: return "Internal Server Error";
            case 501: return "Not Implemented";
            case 502: return "Bad Gateway";
            case 503: return "Service Unavailable";
            case 504: return "Gateway Timeout";
            case 505: return "HTTP Version Not Supported";
            case 506: return "Variant Also Negotiates";
            case 507: return "Insufficient Storage";
            case 508: return "Loop Detected";
            case 510: return "Not Extended";
            case 511: return "Network Authentication Required";

            default:  return "Unknown Status Code";
        }
    }

    // 获取文件后缀对应的MIME(Content-Type)
    static std::string getMimeType(const std::string& filename) 
    {
        // 统一转换为小写处理
        auto lower_ext = [](const std::string& s) ->std::string{
            size_t dot_pos = s.find_last_of('.');
            if (dot_pos == std::string::npos) return "";
            
            std::string ext = s.substr(dot_pos + 1);//获取文件的扩展名
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext;
        };

        // 常见 MIME 类型映射表
        static const std::unordered_map<std::string, std::string> mime_types = {
            // 文本类型
            {"txt",  "text/plain"},
            {"html", "text/html"},
            {"htm",  "text/html"},
            {"css",  "text/css"},
            {"js",   "text/javascript"},
            {"csv",  "text/csv"},

            // 图片类型
            {"jpg",  "image/jpeg"},
            {"jpeg", "image/jpeg"},
            {"png",  "image/png"},
            {"gif",  "image/gif"},
            {"webp", "image/webp"},
            {"svg",  "image/svg+xml"},
            {"ico",  "image/x-icon"},

            // 应用类型
            {"pdf",  "application/pdf"},
            {"json", "application/json"},
            {"xml",  "application/xml"},
            {"zip",  "application/zip"},
            {"gz",   "application/gzip"},
            {"doc",  "application/msword"},
            {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {"xls",  "application/vnd.ms-excel"},
            {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
            {"ppt",  "application/vnd.ms-powerpoint"},
            {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},

            // 音视频类型
            {"mp3",  "audio/mpeg"},
            {"wav",  "audio/wav"},
            {"mp4",  "video/mp4"},
            {"webm", "video/webm"},
            {"ogg",  "video/ogg"}
        };

        std::string ext = lower_ext(filename);
        auto it = mime_types.find(ext);
        if(it == mime_types.end())
        {
            //找不到，默认返回二进制流类型
            return "application/octet-stream";
        }
        return it->second;
    }

    // 判断一个文件是否为目录
    static bool isDirectory(const std::string& path)
    {
        return std::filesystem::is_directory(path);
    }
    // 判断一个文件是否为普通文件
    static bool isRegularFile(const std::string& path)
    {
        return std::filesystem::is_regular_file(path);
    }

    // 判断一个请求资源路径是否合法 (合法指的是请求相对根目录'/'下的资源路径)
    // 如：/index.html 合法
    // 如：/../../etc/passwd 非法
    static bool isVaildPath(const std::string path)
    {
        // 计算路径深度，以'/'为分割，遇到'..'减一，遇到'.'不变，遇到其他加一

        // 1.分割路径
        std::vector<std::string> dirs;
        split(path, "/", dirs);
 
        // 2.遍历每个子目录，以计算路径深度
        int depth = 0;
        for(int i = 0; i < dirs.size(); i++)
        {
            if(dirs[i] == ".")
            {
                continue;
            }
            else if(dirs[i] == "..")
            {
                if(--depth < 0)
                {
                    // 一旦深度小于0，就是非法路径，因为已经到相对根目录的上层目录去了
                    return false;
                }
            }
            else
            {
                depth++;
            }
        }
        return true;
    }

};//Util