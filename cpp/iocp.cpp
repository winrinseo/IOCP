#include "iocp.h"
#include "clientSession.h"
#include <iostream>

Iocp::Iocp(int port)
    : port_(port), listenSocket_(INVALID_SOCKET), iocpHandle_(NULL) {}

Iocp::~Iocp() {
    Cleanup();
}

bool Iocp::Start() {
    if (!InitWinsock()) return false;
    if (!CreateListenSocket()) return false;
    if (!CreateIocp()) return false;

    // 워커 쓰레드 생성
    for (int i = 0; i < WORKER_COUNT; ++i) {
        workerThreads_.emplace_back([this]() { WorkerThread(); });
    }

    // 클라이언트 accept 처리 루프
    AcceptLoop();
    return true;
}

bool Iocp::InitWinsock() {
    WSADATA wsa;
    // Winsock 초기화
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}

bool Iocp::CreateListenSocket() {
    listenSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket_ == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) return false;

    return true;
}

bool Iocp::CreateIocp() {
    // IOCP 포트 생성
    iocpHandle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    return iocpHandle_ != NULL;
}

void Iocp::AcceptLoop() {
    // 클라이언트를 무한히 받아들임
    while (true) {
        SOCKET clientSock = accept(listenSocket_, NULL, NULL);
        if (clientSock == INVALID_SOCKET) continue;

        // 새 클라이언트 세션 생성
        auto session = new ClientSession(clientSock);
        clients_.insert(session);

        // 클라이언트 소켓을 IOCP에 등록
        CreateIoCompletionPort((HANDLE)clientSock, iocpHandle_, (ULONG_PTR)session, 0);

        // 클라이언트로부터 비동기 수신 시작
        session->Receive();
    }
}

void Iocp::WorkerThread() {
    DWORD bytesTransferred;
    ULONG_PTR key;
    LPOVERLAPPED overlapped;

    while (true) {
        // 완료된 IO 이벤트를 기다림
        BOOL result = GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &key, &overlapped, INFINITE);
        auto session = reinterpret_cast<ClientSession*>(key);

        // 실패하거나 연결이 끊어졌다면 종료 처리
        if (!result || bytesTransferred == 0) {
            closesocket(session->GetSocket());
            delete session;
            continue;
        }

        // 수신된 데이터를 처리
        session->OnReceive(bytesTransferred);
    }
}

void Iocp::Cleanup() {
    // 모든 세션 삭제
    for (auto session : clients_) delete session;
    clients_.clear();

    // 리소스 정리
    closesocket(listenSocket_);
    CloseHandle(iocpHandle_);
    WSACleanup();
}
