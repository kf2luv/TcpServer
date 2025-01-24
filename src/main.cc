#include "server.hh"

struct TestCls
{
    TestCls() {}
    TestCls(int aa, int bb) : a(aa), b(bb) {}
    void show()
    {
        std::cout << a << " " << b << std::endl;
    }

    int a;
    double b;
};

void buffer_test()
{
    // Buffer buf(1024);
    // TestCls tc1;
    // for (int i = 0; i < 70; i++)
    // {
    //     // if (i == 30)
    //     // {
    //     //     TestCls tc3;
    //     //     buf.read(&tc3, sizeof(TestCls));
    //     // }
    //     tc1.a = i;
    //     tc1.b = i + 1;

    //     buf.write(&tc1, sizeof(TestCls));
    //     // std::cout << "tc1: ";
    //     // tc1.show();
    // }

    // size_t bytes = buf.readableBytes();

    // for (int i = 0; i < bytes / sizeof(TestCls); i++)
    // {
    //     TestCls tc2;
    //     buf.read(&tc2, sizeof(TestCls));
    //     tc2.show();
    // }

    // Buffer buf;
    // buf.writeString(std::string("长安大学"));
    // buf.writeString(std::string("清华大学"));

    // // std::string s = buf.readAsString(buf.readableBytes());
    // // std::cout << s << std::endl;
    // // std::cout << buf.readableBytes() << std::endl;

    // std::cout << "-----buf2-----" << std::endl;
    // Buffer buf2;
    // buf2.writeBuffer(buf);
    // std::string s2 = buf2.readAsString(buf2.readableBytes());
    // std::cout << s2 << std::endl;

    Buffer buf;
    for (int i = 0; i < 100; i++)
    {
        std::string str = "长安大学" + std::to_string(i) + '\n';
        buf.writeString(str);
    }
    while (!buf.empty())
    {
        std::string line = buf.getLine();
        std::cout << line;
    }
}

int main()
{
    buffer_test();

    return 0;
}