#include "../src/server.hh"

int main(){
    Socket cli_sock;
    cli_sock.Create();
    cli_sock.Connect("127.0.0.1", 8888);
    std::string msg = "X";
    while(true) {
        sleep(1);
        int ret = cli_sock.NonBlockSend(msg.c_str(), msg.size());
        if(ret < 0){
            break;
        } else if(ret == 0){
            continue;
        } else{
            msg += 'X';
        }
    }
    return 0;
}