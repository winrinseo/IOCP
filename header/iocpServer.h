#pragma once

#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <thread>
#include <unordered_set>

#include "iocp.h"


// IOCP 기반 서버를 담당하는 클래스
class IocpServer : public Iocp {
public:
    IocpServer(uint16_t port);
    IocpServer(uint16_t port , uint32_t thread_size);
    ~IocpServer();

    bool Start() override;                           // 서버 시작

private:
    
};
