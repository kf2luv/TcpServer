#include <iostream>
#include "http.hh"

int main()
{
    HttpServer svr(8777);
    svr.SetBaseDir("/home/kf/tcp-server/http/wwwroot");

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
        response._body = "TEST KEEP ALIVE";
        response.setHeader("Content-Type", "text/plain");
        return;
    });


    svr.Listen();
    return 0;
}