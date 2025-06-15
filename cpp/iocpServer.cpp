#include "iocpServer.h"
#include "clientSession.h"
#include <iostream>

IocpServer::IocpServer(int port)
    : port_(port), listenSocket_(INVALID_SOCKET), iocpHandle_(NULL) {}

IocpServer::~IocpServer() {
    Cleanup();
}

bool IocpServer::Start() {
    if (!InitWinsock()) return false;
    if (!CreateListenSocket()) return false;
    if (!CreateIocp()) return false;

    // 워커 쓰레드 생성
    for (int i = 0; i < WORKER_COUNT; ++i) {
        workerThreads_.emplace_back([this]() { WorkerThread(); });
    }

    // 클라이언트 accept 처리 루프
    workerThreads_.emplace_back([this]() { AcceptLoop(); });
    return true;
}

bool IocpServer::InitWinsock() {
    WSADATA wsa;
    // Winsock 초기화
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}

//리슨 소켓 생성 (클라이언트의 연결 요청을 받는 소켓)
bool IocpServer::CreateListenSocket() {
    //소켓 함수 호출, SOCKET은 파일 디스크럽터
    listenSocket_ = socket(AF_INET, SOCK_STREAM, 0); // IPv4 , tcp 통신 (udp라면 SOCK_DGRAM)
    if (listenSocket_ == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET; //IPv4주소
    addr.sin_port = htons(port_); //사용할 포트
    addr.sin_addr.s_addr = INADDR_ANY; //ip할당 INADDR_ANY은 자동 설정

    //소켓에 정보(주소) 할당 
    if (bind(listenSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;
    //소켓에 정보 할당 후 클라이언트가 연결할 수 있도록 대기하는 상태로 만들어줌
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) return false; //소켓 디스크럽터 번호 , 연결 요청 대기 큐의 크기

    return true;
}

//IOCP 생성
bool IocpServer::CreateIocp() {
    // IOCP 포트 생성
    iocpHandle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    return iocpHandle_ != NULL;
}

//클라이언트 요청 수락 루프
void IocpServer::AcceptLoop() {
    // 클라이언트를 무한히 받아들임
    while (true) {
        //해당 리슨 소켓으로 연결 요청을 수락, 클라이언트 소켓의 디스크럽터 획득
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

//작업 스레드
void IocpServer::WorkerThread() {
    DWORD bytesTransferred;
    ULONG_PTR key;
    LPOVERLAPPED overlapped;

    while (true) {
        // 완료된 IO 이벤트를 기다림
        BOOL result = GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &key, &overlapped, INFINITE);
        ClientSession * session = reinterpret_cast<ClientSession*>(key); //키를 세션 주소로 변환
        /* memo
        key는 소켓을 iocp에 등록했을 때 함께 넣었던 식별용 값이라 소켓의 번호를 넣어도 완전히 정상적인 동작을 함
        여기서는 클라이언트 세션을 제어할 수 있는 객체의 주소값을 식별자로 넣음
        */
       IoContext* context = reinterpret_cast<IoContext*>(overlapped); //overlapped 주소 옆에 IoContext의 정보가 보존되어 있기 때문에 해당 캐스팅이 성립함

        // 실패하거나 연결이 끊어졌다면 종료 처리
        if (!result || bytesTransferred == 0) {
            closesocket(session->GetSocket());
            delete session;
            continue;
        }

        //완료란 송수신 받고 난 후를 뜻함. 그래서 completion port

        if (context->operation == OperationType::RECV) { //수신 완료일때
            //수신된 데이터를 처리 및 재수신 준비
            session->OnReceiveCompletion(context->buffer, bytesTransferred);
        } else if (context->operation == OperationType::SEND) {  //송신 완료했을때
            session->OnSendCompletion();
        }

        //메모리 해제
        delete context;
    }
}

void IocpServer::Cleanup() {
    // 모든 세션 삭제
    for (auto session : clients_) delete session;
    clients_.clear();

    // 리소스 정리
    closesocket(listenSocket_);
    CloseHandle(iocpHandle_);
    WSACleanup();
}
