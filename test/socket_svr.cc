#include "../src/server.hh"
#include <thread>

int main() {

    Socket list_sock;
    if(!list_sock.CreateServer(8888))
    {
        std::cerr << "Create Server error" << std::endl;
        return 1;
    }
    // std::vector<std::thread> workers;
    while(true) {
        int newfd = list_sock.Accept();
        if(newfd < 0) {
            if(newfd == -2){
                continue;
            }
            std::cerr << "accept error" << std::endl;
            return 2;
        }

        std::thread worker([newfd](){
            Socket conn_sock(newfd);
            char buffer[64]{};
            while(true) {
                int ret = conn_sock.NonBlockRecv(buffer, 63);
                if(ret < 0){
                    std::cerr << "连接已关闭" << std::endl;
                    break;
                } else if(ret == 0){
                    continue;
                } else {
                    std::cout << "echo: " << buffer << std::endl;
                }
            }
        });
        worker.detach();
    }

    return 0;
}