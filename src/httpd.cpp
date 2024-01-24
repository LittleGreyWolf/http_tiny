#include "httpd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include "json11.hpp"

#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#endif

bool Httpd::start(){
#ifdef _WIN32
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		std::cout << "WSAStartup() error!" << std::endl;
		return false;
	}
#endif
#ifdef _WIN32
	SOCKET server_sockfd;
#else
	int server_sockfd;
#endif


    //定义sockfd
    server_sockfd = socket(AF_INET,SOCK_STREAM, 0);
    if(server_sockfd == -1){
        std::cout << "socket error !" << std::endl;
		return false;
    }
 
    ///定义sockaddr_in
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(4000);
#ifdef _WIN32
    //server_sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    server_sockaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#else
    //server_sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    
 
    //bind，成功返回0，出错返回-1
    if(bind(server_sockfd,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr))==-1){
        perror("bind");
        return false;
    }
 
    //listen，成功返回0，出错返回-1
    if(listen(server_sockfd, 5) == -1){
        perror("listen");
        return false;
    }

    //客户端套接字
    struct sockaddr_in client_addr;

#ifdef _WIN32
    int length = sizeof(client_addr);
#else
    socklen_t length = sizeof(client_addr);
#endif

     // 将监听的fd的状态检测委托给内核检测
    int maxfd = server_sockfd;
    // 初始化检测的读集合
    fd_set rdset;
    fd_set rdtemp;
    // 清零
    FD_ZERO(&rdset);
    // 将监听的lfd设置到检测的读集合中
    FD_SET(server_sockfd, &rdset);

    while(1){
        rdtemp = rdset;
        int num = select(maxfd + 1, &rdtemp, NULL, NULL, NULL);
        if (FD_ISSET(server_sockfd, &rdtemp)) {
            // 接受连接请求, 这个调用不阻塞
            int cfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &length);
            printf("[%s:%d] client connect...\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

            // 得到了有效的文件描述符
            // 通信的文件描述符添加到读集合
            // 在下一轮select检测的时候, 就能得到缓冲区的状态
            FD_SET(cfd, &rdset);
            // 重置最大的文件描述符
            maxfd = cfd > maxfd ? cfd : maxfd;
        }

        // 没有新连接, 通信
        for (int i = 0; i < maxfd + 1; ++i) {
            // 判断从监听的文件描述符之后到maxfd这个范围内的文件描述符是否读缓冲区有数据
            if (i != server_sockfd && FD_ISSET(i, &rdtemp)) {
				disposeFunc((int *)&i, &rdset);
            }
        }
    }
#ifdef _WIN32
	closesocket(server_sockfd);
	WSACleanup();
#else
	close(server_sockfd);
#endif
    return true;
}

void Httpd::disposeFunc(int *csocket, fd_set *rdset) {
	// 接收http请求
	char bodyBuf[1024 * 100] = { 0 };
	int recvSize = recv(*csocket, bodyBuf, sizeof(bodyBuf), 0);
	if (recvSize < 0) {
		printf("recv data failed.\n");
		FD_CLR(*csocket, rdset);
#ifdef _WIN32
		closesocket(*csocket);
#else
		close(*csocket);
#endif
		return;
	}
	else if (recvSize == 0) {
		printf("client disconnect.\n");
		FD_CLR(*csocket, rdset);
#ifdef _WIN32
		closesocket(*csocket);
#else
		close(*csocket);
#endif
		return;
	}

	printf("bodyBuf size = %d\n", strlen(bodyBuf));
	printf("%s\n", bodyBuf);

	std::string strMethod;
	std::string strUri;
	std::string strVersion;
	std::map<std::string, std::string> requestHead;
	std::string requestBody;
	// 解析http请求，包括请求方式(目前只支持GET和POST请求)，URI，http版本
	parseHttpRequestInfo(bodyBuf, strMethod, strUri, strVersion);

	// 若请求方式解析失败，说明非http格式数据
	if (strMethod.empty()) {
		// 直接使用原数据响应
		send(*csocket, bodyBuf, strlen(bodyBuf), 0);
		return;
	}

	// 解析http请求头
	parseHttpRequestHead(bodyBuf, requestHead);
	//解析http请求体
	if (strMethod.compare("POST") == 0) {
		parseHttpRequestBody(bodyBuf, requestBody);
	}
	
	//根据不同请求方式进行响应
	if (strMethod.compare("GET") == 0) {
		httpResponseHtml(*csocket);
	}else if (strMethod.compare("POST") == 0) {
		httpResponseJson(*csocket);
	}

	FD_CLR(*csocket, rdset);
#ifdef _WIN32
	closesocket(*csocket);
#else
	close(*csocket);
#endif
}

bool Httpd::parseHttpRequestInfo(std::string httpRequest, std::string& method, std::string& uri, std::string& version){
    int recvSize = httpRequest.size();
    
    //查找请求头
    std::string strRequestHead;
    int pos = httpRequest.find("\r\n");
    strRequestHead = httpRequest.substr(0, pos);

    //解析请求类型
    method = strRequestHead.substr(0, strRequestHead.find(" "));
    //解析uri
    uri = strRequestHead.substr(strRequestHead.find(" ") + 1, strRequestHead.find(" ", strRequestHead.find(" ") + 1) - strRequestHead.find(" "));
    //解析http版本
    version = strRequestHead.substr(strRequestHead.rfind(" "), strRequestHead.size() - strRequestHead.rfind(" "));
    return true;
}


bool Httpd::parseHttpRequestHead(std::string httpRequest, std::map<std::string, std::string>& requestHead){
    int recvSize = httpRequest.size();
    int headPos = httpRequest.find("\r\n");

    int bodySize = parseBodySize(httpRequest);

    std::string strRequestH;
    do{
        int iPos = httpRequest.find("\r\n", headPos + strlen("\r\n"));
        strRequestH = httpRequest.substr(headPos, iPos - headPos);
        if(strRequestH.find(":") != std::string::npos){
            std::string strKey = strRequestH.substr(0, strRequestH.find(":"));
            std::string strValue = strRequestH.substr(strRequestH.find(":") + 1, strRequestH.size() - strRequestH.find(":"));
            requestHead.insert(std::pair<std::string, std::string>(strKey, strValue));
        }
        headPos = iPos;
    } while(headPos < recvSize - bodySize && headPos > 0);

    return true;
}

bool Httpd::parseHttpRequestBody(std::string httpRequest, std::string& requestBody){
    int recvSize = httpRequest.size();

    int bodySize = parseBodySize(httpRequest);
    if(bodySize == 0){
        return false;
    }

    requestBody = httpRequest.substr(recvSize - bodySize, bodySize);
    return true;
}

int Httpd::parseBodySize(std::string httpRequest){
    std::string strContentLength;
    int posLengthStart = httpRequest.find("Content-Length: ") + strlen("Content-Length: ");
    int posLengthEnd = httpRequest.find("\r\n", httpRequest.find("Content-Length: ") + strlen("Content-Length: "));
    strContentLength =  httpRequest.substr(posLengthStart, posLengthEnd - posLengthStart);
    return atoi(strContentLength.c_str());
}

bool Httpd::httpResponseJson(int client){
    char recvBuf[1024] = {0};
    sprintf(recvBuf, "HTTP/1.0 200 OK\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "Content-type: application/json\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    json11::Json myJson = json11::Json::object {
        { "code", 0 },
        { "msg", "success" },
        { "data", "https://blog.csdn.net/new9232" },
    };

    sprintf(recvBuf, myJson.dump().c_str());
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    return true;
}

bool Httpd::httpResponseHtml(int client){

    char recvBuf[1024] = {0};
    sprintf(recvBuf, "HTTP/1.0 200 OK\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "<HTML>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "<TITLE>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "HTTP Server\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "</TITLE>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "<BODY>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "<div id=\"container\" style=\"width:500px\">\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);
    
    sprintf(recvBuf, "<div id=\"header\" style=\"background-color:#FFA500;\">\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "<h1 style=\"margin-bottom:0;\">www.baidu.com</h1></div>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "<div id=\"menu\" style=\"background-color:#FFD700;height:200px;width:100px;float:left;\">\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "<b>MENU</b><br>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "HTML<br>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "CSS<br>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "JavaScript</div>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "<div id=\"content\" style=\"background-color:#EEEEEE;height:200px;width:400px;float:left;\">BODY</div>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "<div id=\"footer\" style=\"background-color:#FFA500;clear:both;text-align:center;\">runoob.com</div>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);


    sprintf(recvBuf, "</div>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "</BODY>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    sprintf(recvBuf, "</HTML>\r\n");
    send(client, recvBuf, strlen(recvBuf), 0);

    return true;
}