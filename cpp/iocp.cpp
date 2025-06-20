#include "iocp.h"
#include <iostream>

Iocp::Iocp(int port) : 
 port_(port), listenSocket_(INVALID_SOCKET), iocpHandle_(NULL) {}

 Iocp::Iocp() {}

Iocp::~Iocp() {
    Cleanup();
}


// Winsock 초기화
bool Iocp::InitWinsock() {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}

//IOCP 생성
bool Iocp::CreateIocp() {
    // IOCP 포트 생성
    iocpHandle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    return iocpHandle_ != NULL;
}

//리슨 소켓 생성 (클라이언트의 연결 요청을 받는 소켓)
bool Iocp::CreateListenSocket() {
    listenSocket_ = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket_ == INVALID_SOCKET) {
        std::cerr << "listenSocket 생성 실패: " << WSAGetLastError() << "\n";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind 실패: " << WSAGetLastError() << "\n";
        return false;
    }

    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen 실패: " << WSAGetLastError() << "\n";
        return false;
    }

    return true;
}

//iocp 시작
bool Iocp::Start() {
    if (!InitWinsock()) return false;
    if (!CreateIocp()) return false;
    if (!CreateListenSocket()) return false;

    for (int i = 0; i < WORKER_COUNT; ++i) {
        workerThreads_.emplace_back([this]() { AcceptLoop(); });
    }

    // 워커 쓰레드 생성
    for (int i = 0; i < WORKER_COUNT; ++i) {
        workerThreads_.emplace_back([this]() { WorkerThread(); });
    }

    /* AcceptEx 관련 코드
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
    for(int i = 0;i<WORKER_COUNT * 2;i++)
        PostAccept();
    */
    return true;
}


//클라이언트 비동기 연결 예약 (환경이 문제인지는 모르겠는데 안되서 나중에 해야지)
void Iocp::PostAccept() {
    // 더미
    SOCKADDR_IN client_addr;
    int addr_len = sizeof(client_addr);
    ZeroMemory(&client_addr, addr_len);

    // 비동기 소켓 생성
    SOCKET clientSock = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (clientSock == INVALID_SOCKET) {
        std::cerr << "클라이언트 소켓 생성 실패: " << WSAGetLastError() << "\n";
        return;
    }

    // 세션 생성 및 소켓 등록
    ClientSession * session = new ClientSession(clientSock);
    if (CreateIoCompletionPort((HANDLE)clientSock, iocpHandle_, (ULONG_PTR)session, 0) == NULL) {
        std::cerr << "IOCP 등록 실패: " << GetLastError() << "\n";
        closesocket(clientSock);
        delete session;
        return;
    }


    // Accept 객체
    IoContext* context = new IoContext();
    context->wsaBuf.buf = context->buffer;
    context->wsaBuf.len = sizeof(context->buffer);
    context->operation = OperationType::ACCEPT;
    ZeroMemory(&context->overlapped , sizeof(context->overlapped));


    // AcceptEX 예약
    BOOL ret = pAcceptEx(
        listenSocket_,
        clientSock,
        context->buffer,
        0,
        sizeof(SOCKADDR_IN) + 16,
        sizeof(SOCKADDR_IN) + 16,
        NULL,
        &context->overlapped
    );


    if (!ret && WSAGetLastError() != ERROR_IO_PENDING) {
        std::cerr << "AcceptEx 실패: " << WSAGetLastError() << "\n";
        closesocket(clientSock);
        // delete context;
        delete session;
        return;
    }

    std::cout << "[Pre-AcceptEx] listenSocket=" << listenSocket_
    <<", IOCP handle= "<<iocpHandle_
          << ", clientSock=" << clientSock
          << ", overlapped ptr=" << &context->overlapped << "\n";
          
}


//클라이언트 요청 수락 루프
void Iocp::AcceptLoop() {
    // 클라이언트를 무한히 받아들임
    while (true) {
        //해당 리슨 소켓으로 연결 요청을 수락, 클라이언트 소켓의 디스크럽터 획득
        SOCKET clientSock = accept(listenSocket_, NULL, NULL);
        if (clientSock == INVALID_SOCKET) continue;

        // 새 클라이언트 세션 생성
        auto session = new ClientSession(clientSock);
        connects_.insert(session);

        // 클라이언트 소켓을 IOCP에 등록
        CreateIoCompletionPort((HANDLE)clientSock, iocpHandle_, (ULONG_PTR)session, 0);

        // 클라이언트로부터 비동기 수신 시작
        session->Receive();
    }
}




//작업 스레드
void Iocp::WorkerThread() {
    DWORD bytesTransferred;
    ULONG_PTR key;
    LPOVERLAPPED overlapped;

    while (true) {
        // 완료된 IO 이벤트를 기다림
        BOOL result = GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &key, &overlapped, INFINITE);
        
        Session * session = reinterpret_cast<Session*>(key); //키를 세션 주소로 변환

        /* memo
        key는 소켓을 iocp에 등록했을 때 함께 넣었던 식별용 값이라 소켓의 번호를 넣어도 완전히 정상적인 동작을 함
        여기서는 클라이언트 세션을 제어할 수 있는 객체의 주소값을 식별자로 넣음
        */
        IoContext* context = reinterpret_cast<IoContext*>(overlapped); //overlapped 주소 시작 지점에서 순서대로 IoContext의 정보가 보존되어 있기 때문에 해당 캐스팅이 성립함
        
        // 실패하거나 연결이 끊어졌다면 종료 처리
        if (!result || bytesTransferred == 0) {
            std::cerr<<"연결 종료"<<"\n";
            closesocket(session->GetSocket());
            connects_.erase(session);
            delete session;
            continue;
        }
        
        //완료란 송수신 받고 난 후를 뜻함. 그래서 completion port
        if (context->operation == OperationType::RECV) { //수신 완료일때
            
            //해당 세션은 재수신 준비
            if(this->connects_.find(session) != this->connects_.end()) 
                session->Receive();

            // 수신 완료시 수행 할 작업
            OnReceiveCompletion(session , context->buffer, bytesTransferred);

        } else if (context->operation == OperationType::SEND) {  //송신 완료했을때
            OnSendCompletion();
        }else if(context->operation == OperationType::ACCEPT){

            if (setsockopt(session->GetSocket(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
               (char*)&listenSocket_, sizeof(SOCKET)) == SOCKET_ERROR) {
                    std::cerr << "SO_UPDATE_ACCEPT_CONTEXT 실패: " << WSAGetLastError() << "\n";
                    closesocket(session->GetSocket());
                    delete session;
                    delete context;
                    continue;
            }

            this->connects_.insert(session);
            
            // 클라이언트로부터 비동기 수신 시작
            if(this->connects_.find(session) != this->connects_.end()) 
                session->Receive();

            std::cout<<"please"<<"\n";
            // 다시 연결 걸기
            PostAccept();
        }

        //메모리 해제
        delete context;
    }
}

void Iocp::Cleanup() {
    // 모든 세션 삭제
    for (auto session : connects_) delete session;
    connects_.clear();

    // 리소스 정리
    closesocket(listenSocket_);
    CloseHandle(iocpHandle_);
    WSACleanup();
}



// 수신 완료 후 처리
void Iocp::OnReceiveCompletion(Session * session , const char * buffer , DWORD bytesTransferred) {

    //대충 여기서 수신 후 연산 수행
    try{
        ReceiveProcess(session , buffer , bytesTransferred);
    }catch(...){
        std::cerr<<"ReceiveProcess 부재"<<"\n";
    }
}

void Iocp::OnSendCompletion(){
    try{
        SendProcess();
    }catch(...){
        // std::cerr<<"SendProcess 부재"<<"\n";
    }
};

void Iocp::SetReceiveProcess(std::function<void(Session * session , const char * buffer , DWORD bytesTransferred)> f){
    ReceiveProcess = f;
}

void Iocp::SetSendProcess(std::function<void()> f){
    SendProcess = f;
}