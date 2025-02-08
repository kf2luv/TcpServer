#include "EchoServer.hh"

int main()
{
    EchoServer server(8888);
    server.Start();
    return 0;
}