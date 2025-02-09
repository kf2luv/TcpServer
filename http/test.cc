#include "http.hh"

// 辅助函数：打印 vector<string>
void printVector(const std::vector<std::string> &vec)
{
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        std::cout << "\"" << vec[i] << "\"";
        if (i != vec.size() - 1)
            std::cout << ", ";
    }
    std::cout << "]";
}

// 测试函数：给定输入 str、sep 以及预期结果 expected，调用 split 并打印对比
void testSplit(const std::string &str, const std::string &sep, const std::vector<std::string> &expected)
{
    std::vector<std::string> result;
    size_t count = Util::split(str, sep, result);
    std::cout << "Test: str = \"" << str << "\", sep = \"" << sep << "\"" << std::endl;
    std::cout << "Expected (" << expected.size() << "): ";
    printVector(expected);
    std::cout << std::endl;
    std::cout << "Result   (" << result.size() << "): ";
    printVector(result);
    std::cout << std::endl;

    if (result == expected)
        std::cout << "PASS" << std::endl;
    else
        std::cout << "FAIL" << std::endl;

    std::cout << "-------------------------" << std::endl;
}

void testReadFile(const std::string &file_path)
{
    std::string data;
    bool ret = Util::readFile(file_path, data);
    if (ret)
    {
        std::cout << "Read file " << file_path << " success" << std::endl;
        std::cout << "File size: " << data.size() << " bytes" << std::endl;
        std::cout << "File content:\n" << data << std::endl;
    }
    else
    {
        std::cout << "Read file " << file_path << " failed" << std::endl;
    }
}

void testWriteFile(const std::string &file_path)
{
    std::string data = "Hello, world!";
    bool ret = Util::writeFile(file_path, data);
    if (ret)
    {
        std::cout << "Write file " << file_path << " success" << std::endl;
    }
    else
    {
        std::cout << "Write file " << file_path << " failed" << std::endl;
    }
}

void testUrl(const std::string url, bool convertSpace = false)
{
    std::cout << "原始URL: " << url << std::endl;
    std::string encode = Util::urlEncode(url, convertSpace);
    std::cout << "编码后: " << encode << std::endl;
    std::string decode = Util::urlDecode(encode, convertSpace);
    std::cout << "解码后: " << decode << std::endl;
}

void testHttpRequest(const std::string& req_str)
{
    // HttpRequest req(req_str);
    // req.debug();
}

int main()
{
    // // 测试用例1：基本用例
    // // 输入："hello,world" 分隔符：","
    // // 预期结果：["hello", "world"]
    // testSplit("hello,world", ",", {"hello", "world"});

    // // 测试用例2：多个分隔符
    // testSplit("a,b,c,d", ",", {"a", "b", "c", "d"});

    // // 测试用例3：连续分隔符
    // testSplit("a,,b", ",", {"a", "b"});

    // // 测试用例4：无分隔符
    // testSplit("abcdef", ",", {"abcdef"});

    // // 测试用例5：空字符串
    // testSplit("", ",", {});

    // // 测试用例6：分隔符在字符串两端
    // testSplit(",abc,def,", ",", {"abc", "def"});

    // // 测试用例7：分隔符包含多个字符（注意：此时 sep 被当作字符集合处理）
    // // 例如，输入 "cabd" 与 sep "ab"，
    // // 按照代码逻辑：第一次查找会在位置1找到 'a'，提取 "c"，然后 begin 跳过 sep.size() 个字符（即2），
    // // 从位置3提取到结尾得到 "d"。预期结果为 ["c", "d"]。
    // testSplit("cabd", "ab", {"c", "d"});

    // // 测试用例8：重复的分隔符字符
    // // 输入："aaaa" 与 sep "aa"
    // // 根据代码逻辑，第一次查找时 find_first_of 会在位置0找到 'a'，提取空串（被忽略），
    // // 然后 begin = 0 + 2 = 2；再从位置2查找，返回2，提取空串，最终结果为空。
    // testSplit("aaaa", "aa", {});

    // testReadFile("./Makefile");
    // testWriteFile("./test.txt");

    // std::string data;
    // Util::readFile("./http.hh", data);
    // Util::writeFile("./test2.hh", data);

    // testUrl("/login?user=kf&passwd=123456&lang=C++");
    // testUrl("/login?user=k f&passwd=123456&lang=C++", true);

    // std::cout << Util::getStatusDesc(404) << std::endl;
    // std::cout << Util::getStatusDesc(999) << std::endl;

    // std::cout << Util::getMimeType("test.txt") << std::endl;
    // std::cout << Util::getMimeType("test.jpg") << std::endl;
    // std::cout << Util::getMimeType("test.aaa") << std::endl;

    // std::cout << Util::isDirectory("../logger") << std::endl;
    // std::cout << Util::isDirectory("./test.txt") << std::endl;
    // std::cout << Util::isRegularFile("../logger") << std::endl;
    // std::cout << Util::isRegularFile("./test.txt") << std::endl;
    // std::cout << Util::isRegularFile(".") << std::endl;
    // std::cout << Util::isRegularFile("..") << std::endl;

    
    // std::cout << Util::isVaildPath("/index.html") << std::endl;//1
    // std::cout << Util::isVaildPath("/../index.html") << std::endl;//0
    // std::cout << Util::isVaildPath("/src/../index.html") << std::endl;//1
    // std::cout << Util::isVaildPath("/src/../../index.html") << std::endl;//0
    // std::cout << Util::isVaildPath("/././././index.html") << std::endl;//1
    // std::cout << Util::isVaildPath("/index/..") << std::endl;//1

    std::string req_str = R"(POST /api/login?debug=true HTTP/1.1\r\nHost: api.example.com\r\nContent-Type: application/json\r\nContent-Length: 52\r\n\r\n{"username": "test", "password": "123456"})";
    // testSplit(req_str,"\\r\\n",{"POST /api/login?debug=true HTTP/1.1", "Host: api.example.com", "Content-Type: application/json", "Content-Length: 52", "Authorization: Bearer abc123", "X-Custom-Header: value", "", "{\"username\": \"test\", \"password\": \"123456\"}"});
    // testSplit(req_str, "\\r\\n", {"GET /index.html HTTP/1.1", "Host: www.example.com"});
    testHttpRequest(req_str);
    return 0;
}