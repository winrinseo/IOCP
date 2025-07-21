#include "iocpServer.h"
#include <iostream>

IocpServer::IocpServer(uint16_t port) : Iocp(port) {}
IocpServer::IocpServer(uint16_t port , uint32_t thread_size) : Iocp(port , thread_size) {}

IocpServer::~IocpServer() {
    Cleanup();
}

bool IocpServer::Start() {
    if (!InitWinsock()) return false;
    if (!CreateIocp()) return false;
    if (!CreateListenSocket()) return false;
    SetMessageManager();

    if (CreateIoCompletionPort((HANDLE)listenSocket_, iocpHandle_, LISTENSOCKET_ID, 0) == NULL) {
        std::cerr << "리슨 소켓을 IOCP에 등록 실패: " << GetLastError() << "\n";
        return false;
    }

    // 서버 동작 중엔 RPC 등록 불가능
    messageManager.RegistRock();

    // 소켓 id 초기화 (클라이언트 id는 10000부터 시작)
    connectId = 10000;
    // 실제로 연결된 클라이언트 수
    connectCount = 10000;

    _thread = true;
    _accept = true;

    // 워커 쓰레드 생성
    for (int i = 0; i < WORKER_COUNT; ++i) {
        workerThreads_.emplace_back([this]() { WorkerThread(); });
    }

    // AcceptEx 관련 코드
    // AcceptEx 함수 포인터 생성
    DWORD bytes = 0;
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    if (WSAIoctl(listenSocket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAcceptEx, sizeof(guidAcceptEx),
        &pAcceptEx, sizeof(pAcceptEx),
        &bytes, NULL, NULL) == SOCKET_ERROR) {
        std::cerr << "AcceptEx 함수 포인터 가져오기 실패: " << WSAGetLastError() << "\n";
        return false;
    }
    

    std::cout << "[디버그] AcceptEx 포인터: " << (void*)pAcceptEx << "\n";

    // 클라이언트 accept 처리 (미리 많이 예약해놓음)
    for(int i = 0;i<WORKER_COUNT; i++)
        PostAccept();
    
    return true;
}