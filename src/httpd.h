#ifndef __HTTPD_H__
#define __HTTPD_H__

#include <string>
#include <map>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#endif

class Httpd{
public:
    bool start();
    //解析http头部信息
    bool parseHttpRequestInfo(std::string httpRequest, std::string& method, std::string& uri, std::string& version);
    //解析http请求头
    bool parseHttpRequestHead(std::string httpRequest, std::map<std::string, std::string>& requestHead);
    //解析http请求体
    bool parseHttpRequestBody(std::string httpRequest, std::string& requestBody);
    //解析报文体长度。没有报文体返回0
    int parseBodySize(std::string httpRequest);

    bool httpResponseJson(int client);
    bool httpResponseHtml(int client);

	// 业务数据处理
    void disposeFunc(int *csocket, fd_set *rdset);
};

#endif