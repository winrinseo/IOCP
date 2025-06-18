#pragma once

#include "iocp.h"

// 연결할 서버의 종류 end는 서버의 수를 나타냄
typedef enum _server{GAME , VOICE , CHAT , end} SERVER;

// IOCP 기반 서버를 담당하는 클래스
class IocpClient : public Iocp {
public:
    IocpClient();
    ~IocpClient();
    bool Start();
    bool Connect(SERVER server , std::string ip , int port);                 // 서버 연결
    
    ServerSession * getServer(SERVER server);
    private:
    // 클라이언트가 여러가지 서버에 연결할 수 있도록 함
    std::vector<ServerSession *> server_list;       //연결된 서버들
};
