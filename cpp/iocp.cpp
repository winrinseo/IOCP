
#include "iocp.h"
#include <iostream>

Iocp::Iocp(uint16_t port) : 
    port_(port), 
    WORKER_COUNT(0) ,
    listenSocket_(INVALID_SOCKET), 
    iocpHandle_(NULL) {}

 Iocp::Iocp(uint16_t port , uint32_t thread_size) : 
    port_(port), 
    WORKER_COUNT(thread_size) , 
    messageManager(thread_size),
    listenSocket_(INVALID_SOCKET), 
    iocpHandle_(NULL) {}

 Iocp::Iocp(uint32_t thread_size) :
    messageManager(thread_size),
    WORKER_COUNT(thread_size) {}

 Iocp::Iocp() :
    WORKER_COUNT(0) {}

Iocp::~Iocp() {
    Cleanup();
}

uint32_t Iocp::GetSessionId(){
    return sessionId;
}

void Iocp::RpcRegist(BaseMessage* reg , std::function<void(BaseMessage*)> f){
    messageManager.regist(reg , f);
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
    
    setPrimaryProceser();
    if(WORKER_COUNT == 0){
        uint32_t num_cores = std::thread::hardware_concurrency();
        if(num_cores == 0) {
            std::cout<<"CPU 코어 수를 감지할 수 없습니다. 기본값 4를 사용합니다."<<std::endl;
            num_cores = 4;
        }else{
            std::cout<<"시스템 CPU 코어 수 : "<<num_cores<<std::endl;
        }

        WORKER_COUNT = num_cores * 2 + 1;
    }

    std::cout<<"생성할 스레드 개수 : "<<WORKER_COUNT<<std::endl;

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

// 메세지 매니저 세팅
void Iocp::SetMessageManager(){
    messageManager.SetSendGroup([this](uint32_t networkGroupId, char* buffer, int size){
        for(uint32_t & sessionId : this->networkGroup[networkGroupId]){
            if(connects_.find(sessionId) != connects_.end())
                connects_[sessionId]->Send(buffer , size);
        }
    });
    
    messageManager.ReplicationRun(5);
}

//iocp 시작
bool Iocp::Start() {
    if (!InitWinsock()) return false;
    if (!CreateIocp()) return false;
    if (!CreateListenSocket()) return false;
    SetMessageManager();
    // 서버 동작 중엔 RPC 등록 불가능
    messageManager.RegistRock();

    // 소켓 id 초기화 (클라이언트 id는 10000부터 시작)
    connectId = 10000;
    // 실제로 연결된 클라이언트 수
    connectCount = 10000;

    _thread = true;
    _accept = true;

    for (int i = 0; i < WORKER_COUNT; ++i) {
        workerThreads_.emplace_back([this]() { AcceptLoop(); });
    }

    // 워커 쓰레드 생성
    for (int i = 0; i < WORKER_COUNT; ++i) {
        workerThreads_.emplace_back([this]() { WorkerThread(); });
    }
    
    return true;
}


//클라이언트 비동기 연결 예약
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
    std::unique_ptr<Session> session = std::make_unique<Session>(clientSock);
    uint32_t clientId = -1;
    {   
        // 클라이언트 id 부여 동기화
        std::lock_guard<std::mutex> idLock(connectMutex);
        clientId = ++connectId;
    }

    // unique_ptr 소유권 이전
    {
        std::lock_guard<std::mutex> idLock(sessionsMutex);
        connects_[clientId] = std::move(session);
    }
    
    if (CreateIoCompletionPort((HANDLE)clientSock, iocpHandle_, (ULONG_PTR)clientId, 0) == NULL) {
        std::cerr << "IOCP 등록 실패: " << GetLastError() << "\n";
        closesocket(clientSock);
        connects_.erase(clientId);
        return;
    }


    // Accept 객체
    IoContext* context = new IoContext();
    ZeroMemory(&context->overlapped , sizeof(context->overlapped));
    context->wsaBuf.buf = context->buffer;
    context->wsaBuf.len = sizeof(context->buffer);
    context->operation = OperationType::ACCEPT;
    context->connectId = clientId;


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
        delete context;
        connects_.erase(clientId);
        return;
    }

    std::cout << "[Pre-AcceptEx] listenSocket=" << listenSocket_
    <<", IOCP handle= "<<iocpHandle_
          << ", clientSock=" << clientSock
          << ", clientId=" << clientId
          << ", overlapped ptr=" << &context->overlapped << "\n";
          
}


//클라이언트 요청 수락 루프
void Iocp::AcceptLoop() {
    // 클라이언트를 무한히 받아들임
    while (_accept) {
        //해당 리슨 소켓으로 연결 요청을 수락, 클라이언트 소켓의 디스크럽터 획득
        SOCKET clientSock = accept(listenSocket_, NULL, NULL);
        if (clientSock == INVALID_SOCKET) continue;

        // 새 클라이언트 세션 생성
        std::unique_ptr<Session> session = std::make_unique<Session>(clientSock);
        uint32_t clientId = -1;
        {   
            // 클라이언트 id 부여 동기화
            std::lock_guard<std::mutex> idLock(connectMutex);
            clientId = ++connectId;
        }
        
        // unique_ptr 소유권 이전
        connects_[clientId] = std::move(session);

        // 클라이언트 소켓을 IOCP에 등록
        CreateIoCompletionPort((HANDLE)clientSock, iocpHandle_, (ULONG_PTR)clientId, 0);

        // 클라이언트로부터 비동기 수신 시작
        connects_[clientId]->Receive();
    }
}




//작업 스레드
void Iocp::WorkerThread() {
    DWORD bytesTransferred;
    ULONG_PTR key;
    LPOVERLAPPED overlapped;

    while (_thread) {
        // 완료된 IO 이벤트를 기다림
        BOOL result = GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &key, &overlapped, INFINITE);
        

        if (!overlapped) {
            std::cerr << "[WorkerThread] 에러: overlapped 포인터가 NULL입니다. 스레드를 종료합니다.\n";
            break;
        }

        uint32_t sessionId = static_cast<uint32_t>(key); //키를 session id로 변환

        /* memo
        key는 소켓을 iocp에 등록했을 때 함께 넣었던 식별용 값이라 소켓의 번호를 넣어도 완전히 정상적인 동작을 함
        여기서는 클라이언트 세션을 제어할 수 있는 객체의 주소값을 식별자로 넣음
        */
        IoContext * context = reinterpret_cast<IoContext*>(overlapped); //overlapped 주소 시작 지점에서 순서대로 IoContext의 정보가 보존되어 있기 때문에 해당 캐스팅이 성립함
        

        // 실패하거나 연결이 끊어졌다면 종료 처리
        if (!result) {
            DWORD error = WSAGetLastError();
            std::cerr << "GQCS 실패, 에러 코드: " << error << " 세션 ID: " << sessionId << "\n";
            // 에러코드 64번은 클라이언트가 강제 종료한것. 
            // 해당 클라이언트에 비동기 작업을 걸어놨는데 연결을 끊어버려서 네트워크 이름이 사라져서 발생.
            closesocket(connects_[sessionId]->GetSocket());
            connects_.erase(sessionId);
            continue;
        }

        if (bytesTransferred == 0 && context->operation == OperationType::RECV) {
            std::cerr << "세션 " << sessionId << " 에서 연결 정상 종료.\n";
            // (세션 정리 로직 - 필요시 동기화 추가)
            closesocket(connects_[sessionId]->GetSocket());
            connects_.erase(sessionId);
            delete context;
            continue;
        }
            
        //완료란 송수신 받고 난 후를 뜻함. 그래서 completion port
        if (context->operation == OperationType::RECV) { //수신 완료일때
            
            //해당 세션은 재수신 준비
            if(this->connects_.find(sessionId) != this->connects_.end()) 
                connects_[sessionId]->Receive();

            // 역직렬화
            BaseMessage * msg = messageManager.Dispatch(context->buffer , bytesTransferred);

            // 원격 프로시저 호출
            messageManager.CallRPC(msg);

            // 재귀적으로 delete하거나 추후 스마트 포인터로 변경
            delete msg;

            // 수신 완료시 수행 할 작업
            OnReceiveCompletion(sessionId , context->buffer, bytesTransferred);

        } else if (context->operation == OperationType::SEND) {  //송신 완료했을때

            // 송신 완료시 수행 할 작업
            OnSendCompletion();

        } else if(context->operation == OperationType::ACCEPT){

            // 실제 소켓 id로 변경
            sessionId = context->connectId;

            // 클라이언트 소켓의 정보 업데이트
            if (setsockopt(connects_[sessionId]->GetSocket(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
            (char*)&listenSocket_, sizeof(SOCKET)) == SOCKET_ERROR) {
                std::cerr << "SO_UPDATE_ACCEPT_CONTEXT 실패: " << WSAGetLastError() << "\n";
                closesocket(connects_[sessionId]->GetSocket());
                delete context;
                continue;
            }
            
            // 클라이언트로부터 비동기 수신 시작
            if(connects_.find(sessionId) != connects_.end()) 
                connects_[sessionId]->Receive();


            // 다시 연결 걸기
            PostAccept();


            // 연결 완료 통보해주기
            std::shared_ptr<Hello> hello = std::make_shared<Hello>();
            hello->sessionId = sessionId;
            Send(sessionId , hello);

        } else if(context->operation == OperationType::CONNECT){
            // 소켓의 컨텍스트를 업데이트
            if (setsockopt(connects_[sessionId]->GetSocket(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) == SOCKET_ERROR) {
                std::cerr << "SO_UPDATE_CONNECT_CONTEXT 실패: " << WSAGetLastError() << "\n";
            } else {
                std::cout << "서버에 성공적으로 연결됨!" << std::endl;
                // 연결 성공 후 첫 Receive 호출
                if(connects_.find(sessionId) != connects_.end())
                    connects_[sessionId]->Receive();
            }
        }

        //메모리 해제
        delete context;
    }
}

// 해당 세션에 메세지 전송
bool Iocp::Send(const uint32_t & session_id , BaseMessage * msg){
    // 메세지 매니저에 직렬화 요청
    char * buffer = nullptr;
    int length = -1;

    messageManager.Serialize(msg, &buffer , &length);

    if(buffer == nullptr || length == -1)
        return false;
    
    connects_[session_id]->Send(buffer , length);
    return true;
}

// shared_ptr 대응 인터페이스
bool Iocp::Send(const uint32_t & session_id , std::shared_ptr<BaseMessage> msg){
    bool ret = Send(session_id , msg.get());
    return ret;
}

// unique_ptr 대응 인터페이스
bool Iocp::Send(const uint32_t & session_id , std::unique_ptr<BaseMessage> msg){
    bool ret = Send(session_id , msg.get());
    return ret;
}


void Iocp::Cleanup() {

    messageManager.RegistUnrock();

    _thread = false;
    _accept = false;
    // 모든 세션 삭제
    connects_.clear();

    // 리소스 정리
    closesocket(listenSocket_);
    CloseHandle(iocpHandle_);
    WSACleanup();
}



// 수신 완료 후 처리
void Iocp::OnReceiveCompletion(uint32_t & sessionKey , const char * buffer , DWORD bytesTransferred) {

    try{
        ReceiveProcess(sessionKey , buffer , bytesTransferred);
    }catch(const std::exception& e){
        std::cerr<<"ReceiveProcess 에러 : "<<e.what()<<"\n";
    }
}

void Iocp::OnSendCompletion(){
    try{
        SendProcess();
    }catch(const std::exception& e){
        std::cerr<<"SendProcess 에러 : "<<e.what()<<"\n";
    }
};

// 기본 RPC 함수를 등록
void Iocp::setPrimaryProceser(){
    // 서버에 연결되었다는 알림림
    messageManager.regist(new Hello() , [this](BaseMessage * msg){
        Hello * hello = (Hello*) msg;
        this->sessionId = hello->sessionId;
        std::cout<<"서버 연결 완료!!"<<std::endl;
    });

    // 서버의 특정 그룹에 연결
    messageManager.regist(new IntoNetworkGroup() , [this](BaseMessage * msg){
        IntoNetworkGroup * data = (IntoNetworkGroup *)msg;
        
        networkGroup[data->networkGroup].push_back(data->sessionId);
    });

    messageManager.regist(new ReplicationData() , [this](BaseMessage * msg){
        ReplicationData * objectData = (ReplicationData*) msg;
        
        GameObjectManager * gm = GameObjectManager::Get();

        for(auto & it : objectData->objList){
            uint32_t & networkId = it->networkId;
            uint32_t & objectId = it->objectId;

            
            if(objectId == 0){ // 오브젝트 아이디가 0으로 오면 삭제
                gm->DeleteObjectFromOther(objectData->networkGroup , networkId);

            }else if(gm->ObjectToAddress(networkId) == nullptr){ // 받는 쪽에서 없으면 생성
                gm->CreateObjectFromOther(objectData->networkGroup,networkId, it->obj);

            }else{ // 있으면 업데이트
                gm->UpdateObjectFromOther(objectData->networkGroup , networkId , it->obj);

            }
        }

    });
}

void Iocp::SetReceiveProcess(std::function<void(uint32_t & sessionKey , const char * buffer , DWORD bytesTransferred)> f){
    ReceiveProcess = f;
}

void Iocp::SetSendProcess(std::function<void()> f){
    SendProcess = f;
}