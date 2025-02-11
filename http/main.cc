#include <iostream>
#include "http.hh"

#define BASE_DIR "/home/kf/tcp-server/http/wwwroot"

int main()
{
    HttpServer svr(8777, 3, true, 100);
    svr.SetBaseDir(BASE_DIR);

    svr.Get("/test1", [](const HttpRequest &request, HttpResponse &response){
        response._stat_code = 200;
        response._body = "TEST GET";
        response.setHeader("Content-Type", "text/plain");
        return;
    });

    svr.Post("/test2", [](const HttpRequest &request, HttpResponse &response){
        response._stat_code = 200;
        response._body = "TEST POST";
        response.setHeader("Content-Type", "text/plain");
        return;
    });

    svr.Post("/login", [](const HttpRequest &request, HttpResponse &response){
        response._stat_code = 200;
        response._body = "LOGIN SUCCESSED, WELCOME!!!";
        response.setHeader("Content-Type", "text/plain");
        return;
    });

    svr.Put("/test3", [](const HttpRequest &request, HttpResponse &response){
        response._stat_code = 200;
        response._body = "TEST PUT";
        response.setHeader("Content-Type", "text/plain");
        return;
    });

    svr.Delete("/test4", [](const HttpRequest &request, HttpResponse &response){
        response._stat_code = 200;
        response._body = "TEST DELETE";
        response.setHeader("Content-Type", "text/plain");
        return;
    });

    svr.Get("/hi", [](const HttpRequest &request, HttpResponse &response){
        response._stat_code = 200;
        response._body = "TEST";
        response.setHeader("Content-Type", "text/plain");
        return;
    });

    svr.Get("/hello", [](const HttpRequest &request, HttpResponse &response){
        sleep(15);//模拟业务超时
        response._stat_code = 200;
        response._body = "TEST";
        response.setHeader("Content-Type", "text/plain");
        return;
    });

    svr.Put("/upload/big.txt", [](const HttpRequest &request, HttpResponse &response){
        //保存文件内容
        std::string path = BASE_DIR;
        path += "/big.txt";
        if(Util::writeFile(path, request._body)){
            response._stat_code = 200;
        }else{
            response._stat_code = 500;
        }
        return;
    });


    svr.Listen();
    return 0;
}