#include "iocpServer.h"
#include <iostream>

IocpServer::IocpServer(int port) : Iocp(port) {}

IocpServer::~IocpServer() {
    Cleanup();
}

// bool IocpServer::Start() {
//     if (!InitWinsock()) return false;
//     if (!CreateIocp()) return false;
//     if (!CreateListenSocket()) return false;

//     _thread = true;
//     _accept = true;

//     workerThreads_.emplace_back([this]() { AcceptLoop(); });

//     // 워커 쓰레드 생성
//     for (int i = 0; i < WORKER_COUNT; ++i) {
//         workerThreads_.emplace_back([this]() { WorkerThread(); });
//     }

//     // 클라이언트 accept 처리 (미리 많이 예약해놓음)
//     // for(int i = 0;i<WORKER_COUNT * 2;i++)
//     //     PostAccept();
//     return true;
// }