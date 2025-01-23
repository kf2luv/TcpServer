#include <iostream>
#include <regex> 

int main() {

    std::string s = "/download/kf/file1";

    std::regex reg("/download/(.*)/(.*)");
    std::smatch sm;
    std::regex_match(s, sm, reg);

    for(auto ret : sm){
        std::cout << ret << std::endl;
    }

    return 0;
}