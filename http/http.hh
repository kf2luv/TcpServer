#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <cstring>
#include "../src/server.hh"

#include "../logger/ckflog.hpp"

class Util
{
public:
    // 分割字符串，以sep作为分隔字符串，对str进行分割，最终结果保存到result中，返回分割后的子串个数
    static size_t split(const std::string &str, const std::string &sep, std::vector<std::string> &result, bool skip_empty = true)
    {
        size_t begin = 0, end = 0;
        while (begin < str.size() && end < str.size())
        {
            end = str.find(sep, begin); // find匹配整个寻找字符，find_first_of匹配任意一个字符
            if (end == std::string::npos)
            {
                end = str.size();
            }
            std::string sub_str = str.substr(begin, end - begin);
            if (skip_empty && sub_str.empty())
            {
                // 跳过空串
                begin = end + sep.size();
                continue;
            }
            result.push_back(sub_str);
            begin = end + sep.size();
        }
        return result.size();
    }

    // 读取文件内容
    static bool readFile(const std::string &file_path, std::string &data)
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
        if (!ifs.good())
        {
            DF_ERROR("Read file %s failed", file_path.c_str());
            return false;
        }

        // 3.关闭文件
        ifs.close();
        return true;
    }

    // 写入文件内容
    static bool writeFile(const std::string &file_path, const std::string &data)
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
        if (!ofs.good())
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
        for (auto &c : url)
        {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') // 普通数字和字母字符，还有非转换符号
            {
                result += c;
            }
            else if (c == ' ' && convertSpace)
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
        // 遇到%就解码(将紧随其后的两个数字转换为字符)，遇到+就替换为空格
        std::string result;
        size_t i = 0;
        while (i < url.size())
        {
            if (url[i] == '+' && convertPlus)
            {
                result += ' ';
                i++;
                continue;
            }

            if (url[i] == '%')
            {
                assert(i + 2 < url.size()); // 至少还有两个字符
                char c = (char)std::stoi(url.substr(i + 1, 2), nullptr, 16);
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
        case 100:
            return "Continue";
        case 101:
            return "Switching Protocols";
        case 102:
            return "Processing";
        case 103:
            return "Early Hints";

        // 2xx Success
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 203:
            return "Non-Authoritative Information";
        case 204:
            return "No Content";
        case 205:
            return "Reset Content";
        case 206:
            return "Partial Content";
        case 207:
            return "Multi-Status";
        case 208:
            return "Already Reported";
        case 226:
            return "IM Used";

        // 3xx Redirection
        case 300:
            return "Multiple Choices";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 303:
            return "See Other";
        case 304:
            return "Not Modified";
        case 305:
            return "Use Proxy";
        // 306 已经被弃用
        case 307:
            return "Temporary Redirect";
        case 308:
            return "Permanent Redirect";

        // 4xx Client Error
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 402:
            return "Payment Required";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 407:
            return "Proxy Authentication Required";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 412:
            return "Precondition Failed";
        case 413:
            return "Payload Too Large";
        case 414:
            return "URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 416:
            return "Range Not Satisfiable";
        case 417:
            return "Expectation Failed";
        case 418:
            return "I'm a teapot"; // 彩蛋
        case 421:
            return "Misdirected Request";
        case 422:
            return "Unprocessable Entity";
        case 423:
            return "Locked";
        case 424:
            return "Failed Dependency";
        case 425:
            return "Too Early";
        case 426:
            return "Upgrade Required";
        case 428:
            return "Precondition Required";
        case 429:
            return "Too Many Requests";
        case 431:
            return "Request Header Fields Too Large";
        case 451:
            return "Unavailable For Legal Reasons";

        // 5xx Server Error
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        case 506:
            return "Variant Also Negotiates";
        case 507:
            return "Insufficient Storage";
        case 508:
            return "Loop Detected";
        case 510:
            return "Not Extended";
        case 511:
            return "Network Authentication Required";

        default:
            return "Unknown Status Code";
        }
    }

    // 获取文件后缀对应的MIME(Content-Type)
    static std::string getMimeType(const std::string &filename)
    {
        // 统一转换为小写处理
        auto lower_ext = [](const std::string &s) -> std::string
        {
            size_t dot_pos = s.find_last_of('.');
            if (dot_pos == std::string::npos)
                return "";

            std::string ext = s.substr(dot_pos + 1); // 获取文件的扩展名
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext;
        };

        // 常见 MIME 类型映射表
        static const std::unordered_map<std::string, std::string> mime_types = {
            // 文本类型
            {"txt", "text/plain"},
            {"html", "text/html"},
            {"htm", "text/html"},
            {"css", "text/css"},
            {"js", "text/javascript"},
            {"csv", "text/csv"},

            // 图片类型
            {"jpg", "image/jpeg"},
            {"jpeg", "image/jpeg"},
            {"png", "image/png"},
            {"gif", "image/gif"},
            {"webp", "image/webp"},
            {"svg", "image/svg+xml"},
            {"ico", "image/x-icon"},

            // 应用类型
            {"pdf", "application/pdf"},
            {"json", "application/json"},
            {"xml", "application/xml"},
            {"zip", "application/zip"},
            {"gz", "application/gzip"},
            {"doc", "application/msword"},
            {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {"xls", "application/vnd.ms-excel"},
            {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
            {"ppt", "application/vnd.ms-powerpoint"},
            {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},

            // 音视频类型
            {"mp3", "audio/mpeg"},
            {"wav", "audio/wav"},
            {"mp4", "video/mp4"},
            {"webm", "video/webm"},
            {"ogg", "video/ogg"}};

        std::string ext = lower_ext(filename);
        auto it = mime_types.find(ext);
        if (it == mime_types.end())
        {
            // 找不到，默认返回二进制流类型
            return "application/octet-stream";
        }
        return it->second;
    }

    // 判断一个文件是否为目录
    static bool isDirectory(const std::string &path)
    {
        return std::filesystem::is_directory(path);
    }
    // 判断一个文件是否为普通文件
    static bool isRegularFile(const std::string &path)
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
        for (int i = 0; i < dirs.size(); i++)
        {
            if (dirs[i] == ".")
            {
                continue;
            }
            else if (dirs[i] == "..")
            {
                if (--depth < 0)
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

}; // Util

// http请求信息模块
class HttpRequest
{
public:
    std::string _method;                                        // 请求方法
    std::string _url;                                           // 请求URL
    std::smatch _matches;                                       // URL正则匹配结果
    std::string _resource_path;                                 // 请求资源路径 (从URL中解析)
    std::string _version;                                       // HTTP版本
    std::string _body;                                          // 请求体
    std::unordered_map<std::string, std::string> _headers;      // 请求头
    std::unordered_map<std::string, std::string> _query_params; // 请求查询参数（从查询字符串中解析）

public:
    // 重置请求信息
    void reset()
    {
        _method.clear();
        _url.clear();
        _version.clear();
        _body.clear();
        _headers.clear();
        _query_params.clear();

        std::smatch sm;
        _matches.swap(sm);
    }

    // 设置头部字段
    void setHeader(const std::string &key, const std::string &val)
    {
        if (key.empty() || val.empty())
        {
            return;
        }
        _headers[key] = val;
    }
    // 获取头部字段
    std::string getHeader(const std::string &key) const
    {
        return hasHeader(key) ? _headers[key] : "";
    }
    // 查看头部字段是否存在
    bool hasHeader(const std::string &key) const
    {
        return _headers.find(key) != _headers.end();
    }

    // 设置查询字符串参数
    void setParam(const std::string &key, const std::string &val)
    {
        if (key.empty() || val.empty())
        {
            return;
        }
        _query_params[key] = val;
    }
    // 获取查询字符串参数
    std::string getParam(const std::string &key) const
    {
        return hasParam(key) ? _query_params[key] : "";
    }
    // 查看查询字符串参数是否存在
    bool hasParam(const std::string &key) const
    {
        return _query_params.find(key) != _query_params.end();
    }

    // 获取请求体长度
    size_t getContentLength()
    {
        if (hasHeader("Content-Length"))
        {
            return std::stol(getHeader("Content-Length"));
        }
        return 0;
    }

    // 判断是否为短连接
    bool close() const
    {
        return !hasHeader("Connection") || getHeader("Connection") == "close";
    }
}; // HttpRequest

// http响应信息模块
class HttpResponse
{
public:
    std::string _version;                                  // 协议版本
    int _stat_code;                                        // 状态码
    std::unordered_map<std::string, std::string> _headers; // 响应头
    std::string _body;                                     // 响应体
    bool _redirect_flag;                                   // 是否重定向
    std::string _redirect_url;                             // 重定向路径（如果有）

public:
    HttpResponse(int stat_code = 200) : _stat_code(stat_code), _redirect_flag(false) {}

    // 重置响应信息
    void reset()
    {
        _stat_code = 0;
        _headers.clear();
        _body.clear();
        _redirect_flag = false;
        _redirect_url.clear();
    }
    // 响应头部的获取
    std::string getHeader(const std::string &key)
    {
        return hasHeader(key) ? _headers[key] : "";
    }
    // 响应头部的设置
    void setHeader(const std::string &key, const std::string &val)
    {
        if (key.empty() || val.empty())
        {
            return;
        }
        _headers[key] = val;
    }

    // 查看响应头部是否存在
    bool hasHeader(const std::string &key) const
    {
        return _headers.find(key) != _headers.end();
    }

    // 设置响应体
    void setBody(const std::string &body, const std::string &type)
    {
        setHeader("Content-Type", type);
        _body = body;
    }

    // 设置重定向（设置重定向状态码和重定向路径）
    void setRedirect(const std::string &redirect_url, int stat_code = 302)
    {
        _stat_code = stat_code;
        _redirect_flag = true;
        _redirect_url = redirect_url;
        // setHeader("Location", redirect_path);
    }

    // 判断是否为短连接
    bool close()
    {
        return !hasHeader("Connection") || getHeader("Connection") == "close";
    }

    // 序列化
    std::string serialize()
    {
        std::ostringstream ss;
        //首行
        ss << _version << " " << _stat_code << " " << Util::getStatusDesc(_stat_code) << "\\r\\n";
        //报头
        for(auto& [k, v] : _headers)
        {
            ss << k << ": " << v << "\\r\\n";
        }
        //空行
        ss << "\\r\\n";
        //正文
        ss << _body;

        return ss.str();
    }

}; // HttpResponse

// 请求接收上下文模块
typedef enum
{
    PARSE_ERROR,        // 解析错误
    PARSE_REQUEST_LINE, // 解析请求行
    PARSE_HEADERS,      // 解析请求头部
    PARSE_BODY,         // 解析请求体
    PARSE_FINISHED,     // 解析完成
} HttpParseStat;

class HttpContext
{
    const static size_t MAX_LINE_LEN = 8192;             // 8KB
    const static size_t MAX_BODY_LEN = 10 * 1024 * 1024; // 10MB

private:
    HttpRequest _request;      // 请求信息
    HttpParseStat _parse_stat; // 当前解析状态
    int _resp_stat_code;       // 响应状态码
public:
    HttpContext() : _parse_stat(PARSE_REQUEST_LINE), _resp_stat_code(200) {}

    // 返回响应状态码
    int getRespStatCode() { return _resp_stat_code; }

    // 返回当前解析状态
    HttpParseStat getParseStat() { return _parse_stat; }

    // 返回请求信息
    HttpRequest &getRequest()
    {
        // assert(_parse_stat == PARSE_FINISHED);
        return _request;
    }

    // 接收并解析HTTP请求
    bool recvAndParseRequest(Buffer &buffer)
    {
        if(_parse_stat == PARSE_FINISHED)
        {
            return true;
        }
        switch (_parse_stat)
        {
            case PARSE_REQUEST_LINE: recvRequestLine(buffer);
            case PARSE_HEADERS: recvHeaders(buffer);
            case PARSE_BODY: recvBody(buffer);
        }
        return _parse_stat != PARSE_ERROR;
    }

    // 重置上下文信息
    void reset()
    {
        _parse_stat = PARSE_REQUEST_LINE;
        _resp_stat_code = 200;
    }

private:
    bool recvRequestLine(Buffer &buffer)
    {
        if (_parse_stat != PARSE_REQUEST_LINE)
        {
            return false;
        }
        // 1.读取buffer中的一行数据
        std::string line = buffer.getLine("\\r\\n");
        if (line.size() == 0)//找不到换行符
        {
            // 缓冲区数据不足一行，且长度过长，设置状态码
            if (buffer.readableBytes() > MAX_LINE_LEN)
            {
                _parse_stat = PARSE_ERROR;
                _resp_stat_code = 414; // URI TOO LONG
                return false;
            }
            // 缓冲区数据不足一行，且长度较短，返回等待新数据到来，凑到一行再来处理
            return true;
        }
        // 从buffer中读到一个完整的请求行
        if (line.size() > MAX_LINE_LEN)
        {
            // 请求行过长
            _parse_stat = PARSE_ERROR;
            _resp_stat_code = 414; // URI TOO LONG
            return false;
        }
        // 2.去除line中的换行符，方便正则表达式匹配
        line.erase(line.size() - strlen("\\r\\n"));

        // 2.解析请求行
        bool ret = parseRequestLine(line);
        if(ret == true)
        {
            _parse_stat = PARSE_HEADERS;// 当前解析成功完成，更改为下一个解析状态
        }
        return false;
    }

    bool parseRequestLine(const std::string &line)
    {
        // 1.使用正则表达式解析，捕获关键要素（忽略大小写）
        static const std::regex http_reg(R"((GET|POST|PUT|DELETE|HEAD|OPTIONS|PATCH) ([^ ?]+)(?:\?([^ ]*))? HTTP\/(\d\.\d)$)", std::regex::icase);
        std::smatch matches;
        bool ok = std::regex_match(line, matches, http_reg);
        if (!ok)
        {
            _parse_stat = PARSE_ERROR;
            _resp_stat_code = 400; // Bad Request
            return false;
        }

        // 2.matches[0]是整个请求行，往后是各项匹配结果
        // 设置httpRequest各项元素，资源路径和查询字符串参数可能会有特殊字符，需要解码
        // 请求方法 （请求方法小写转大写）
        std::string method = matches[1];
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        _request._method = method;

        _request._resource_path = Util::urlDecode(matches[2]); // 资源路径
        if (matches[3].matched)                                // 查询字符串（如果有）
        {
            std::vector<std::string> params;
            Util::split(matches[3].str(), "&", params);
            std::vector<std::string> kv(2);
            for (auto &param : params)
            {
                size_t ret = Util::split(param, "=", kv);
                if (ret != 2)
                {
                    _parse_stat = PARSE_ERROR;
                    _resp_stat_code = 400; // Bad Request
                    return false;
                }
                std::string key = Util::urlDecode(kv[0], true); // 解码，'+' -> ' '
                std::string val = Util::urlDecode(kv[1], true);
                _request.setParam(key, val);
            }
        }
        _request._version = matches[4]; // 协议版本
        return true;
    }

    bool recvHeaders(Buffer &buffer)
    {
        if (_parse_stat != PARSE_HEADERS)
        {
            return false;
        }
        // 1.逐行获取数据
        while (true)
        {
            std::string line = buffer.getLine("\\r\\n");
            if (line.size() == 0) // 没有换行符，不构成一行数据
            {
                if (buffer.readableBytes() > MAX_LINE_LEN)
                {
                    _parse_stat = PARSE_ERROR;
                    _resp_stat_code = 414; // URI TOO LONG
                    return false;
                }
                return true;
            }
            if (line.size() > MAX_LINE_LEN)
            {
                _parse_stat = PARSE_ERROR;
                _resp_stat_code = 414; // URI TOO LONG
                return false;
            }
            if (line == "\\r\\n")
            {
                // 读到空行，代表header处理结束
                return true;
            }

            // 2.解析一个请求头部
            if (parseHeaders(line) == false)
            {
                return false;
            }
        }

        _parse_stat = PARSE_BODY; // 当前解析成功完成，更改为下一个解析状态
        return true;
    }

    bool parseHeaders(const std::string &line)
    {
        // Content-Length: 50
        std::vector<std::string> kv(2);
        size_t ret = Util::split(line, ": ", kv);
        if (ret != 2)
        {
            _parse_stat = PARSE_ERROR;
            _resp_stat_code = 400; // Bad Request
            return false;
        }
        std::string key = kv[0];
        std::string val = kv[1];
        _request.setHeader(key, val);

        return true;
    }

    bool recvBody(Buffer &buffer)
    {
        if (_parse_stat != PARSE_BODY)
        {
            return false;
        }
        // 1.获取请求的正文长度
        size_t content_length = _request.getContentLength();
        if (_request._body.empty())
        {
            // 初次读取正文
            if (content_length == 0)
            {
                // 没有正文
                _parse_stat = PARSE_FINISHED;
                return true;
            }
            if (content_length > MAX_BODY_LEN)
            {
                // 请求正文过大
                _parse_stat = PARSE_ERROR;
                _resp_stat_code = 413; // Payload Too Large
                return false;
            }
        }

        // 2.获取当前已读取的正文长度，判断还差多少
        size_t diff_len = content_length - _request._body.size();

        // 3.读取正文数据
        if (buffer.readableBytes() < diff_len)
        {
            // 不足diff_len，有多少取多少，等待下次新数据到来
            _request._body.append(buffer.readPos(), buffer.readableBytes());
            buffer.moveReadIdx(buffer.readableBytes());
        }
        else
        {
            // 足够，则读取diff_len
            _request._body.append(buffer.readPos(), diff_len);
            buffer.moveReadIdx(diff_len);
            // 读取正文结束
            _parse_stat = PARSE_FINISHED;
        }
        return true;
    }

}; // HttpContext


//HTTP服务器
class HttpServer
{
    using Handler = std::function<void(const HttpRequest &request, HttpResponse &response)>;
    using HandlerMap = std::unordered_map<std::regex, Handler>;

private:
    TcpServer _server;         // 底层IO的tcp服务器
    std::string _base_dir;     // 静态资源根目录

    HandlerMap _get_router;    // GET方法路由表
    HandlerMap _post_router;   // POST方法路由表
    HandlerMap _put_router;    // PUT方法路由表
    HandlerMap _delete_router; // DELETE方法路由表

private:

    //为tcp服务器设置的回调函数


    void onConnected(const PtrConnection& conn)
    {
        //将连接上下文设置为HttpContext
        conn->setContext(HttpContext());
    }


    // 读缓冲区数据到来，进行处理
    // 通过连接的http上下文，接受一个http请求信息
    // 根据请求信息，进行路由，找到对应的handler
    // 调用handler，进行业务处理，并得到一个http响应
    // 通过conn将响应返回给客户端
    // 最后根据连接是否为长连接，判断是否关闭连接
    void onMessage(const PtrConnection & conn, Buffer & buffer)
    {
        while(buffer.readableBytes() > 0) 
        {
            //1.获取连接的上下文信息
            HttpContext* context = conn->getContext()->get<HttpContext>();

            //2.从缓冲区读取并解析一个请求
            bool ok = context->recvAndParseRequest(buffer);
            HttpResponse response(context->getRespStatCode());

            if(!ok && context->getParseStat() == PARSE_ERROR)
            {
                // 组织一个错误显示的页面到响应中
                errorPageResponse(response);
                // 返回响应给客户端（注意此时获取的request是无效的，但是由于后面连接就要关闭了，所以无所谓）
                writeResponse(conn, context->getRequest(), response);
                // 清空缓冲区，连接出错，数据是无效的，避免干扰关闭连接的操作
                buffer.clear();
                // 关闭连接
                conn->shutdown();
                return;
            }
            else if(context->getParseStat() != PARSE_FINISHED)
            {
                //未能从缓冲区中读取一个完整的request，先返回，等待新数据到来再处理
                return;
            }
            assert(context->getParseStat() == PARSE_FINISHED);
            
            //3.成功读取到一个完整的request，走到这里request才是有效的，路由找到业务函数handler
            HttpRequest request = context->getRequest();
            auto handler = route(request, response);

            //4.调用handler进行业务处理
            handler(request, response);

            //5.将响应返回给客户端
            writeResponse(conn, request, response);

            //6.长短连接的判断
            if(request.close())
            {
                //短连接关闭
                conn->shutdown();
                return;
            }
            //长连接，循环处理，先重置上下文
            context->reset();
        }
    }

    // 组织一个错误显示的页面到响应中
    void errorPageResponse(HttpResponse &response)
    {
        // 生成 HTML 错误页面
        std::ostringstream html;
        std::string desc = Util::getStatusDesc(response._stat_code);
        html << "<!DOCTYPE html>\n";
        html << "<html lang=\"en\">\n";
        html << "<head><meta charset=\"UTF-8\"><title>" << response._stat_code << " " << desc << "</title></head>\n";
        html << "<body>\n";
        html << "<h1>" << response._stat_code << " - " << desc << "</h1>\n";
        html << "<p>Sorry, the page you are looking for is not available.</p>\n";
        html << "</body>\n</html>";
        response.setBody(html.str(), "text/html");
    }

    // 为response填充一些必要的信息后，将response对象序列化，通过conn向客户端返回响应
    void writeResponse(const PtrConnection & conn, const HttpRequest& request, HttpResponse& response)
    {
        // 1.为response填充一些必要的信息
        if(request.close()) {
            response.setHeader("Connection", "close");//短连接
        } else{
            response.setHeader("Connection", "keep-alive");//长连接
        }

        if(!response._body.empty() && !response.hasHeader("Content-Length"))
        {
            response.setHeader("Content-Length", std::to_string(response._body.size()));
        }
        if(!response._body.empty() && !response.hasHeader("Content-Type"))
        {
            response.setHeader("Content-Type", "application/octet-stream");
        }
        if(response._redirect_flag == true && !response.hasHeader("Location"))
        {
            response.setHeader("Location", response._redirect_url);
        }
        response._version = request._version.empty() ? "HTTP/1.1" : request._version;//避免request无效的情况

        // 2.将response对象序列化
        std::string resp_str = response.serialize();

        // 3.返回响应
        conn->send(resp_str.c_str(), resp_str.size());
    }


    // 根据request请求信息，从路由表中找到对应的业务处理函数，找不到则通过response返回错误信息Method Not Allowed
    Handler route(const HttpRequest& request, HttpResponse& response);

public:
    HttpServer();

    // 设置各种请求方法的路由项
    void Get(const std::string pattern, const Handler &handler);
    void Post(const std::string pattern, const Handler &handler);
    void Put(const std::string pattern, const Handler &handler);
    void Delete(const std::string pattern, const Handler &handler);
    // 设置工作线程数量
    void SetThreadCount(size_t count);
    // 服务器开始运行
    void Start();
};