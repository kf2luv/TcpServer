// #include "Any.hh"
#include <iostream>
#include <any>

int main(){
    // 自己实现的any
    // Any any(123);
    // int* ptr1 = any.get<int>();
    // std::cout << *ptr1 << std::endl;
    // any = std::string("whatup");
    // std::string s = *any.get<std::string>();
    // std::cout << s << std::endl;

    // C++17中的any
    std::any a = 39;
    std::cout << std::any_cast<int>(a) << std::endl;

    a = std::string("ubuntu");
    std::cout << std::any_cast<std::string>(a) << std::endl;



    return 0;
}