#include "iocpClient.h"


IocpClient::IocpClient(){
    if (!InitWinsock()) {
        std::cerr << "WSAStartup 실패\n";
    }

    if (!CreateIocp()) {
        std::cout<<"iocp 실패"<<"\n";
    }

    //서버 세션 객체 생성
    for(int i = 0;i<SERVER::end;i++){
        SOCKET sock_ = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
        ServerSession * session = new ServerSession(sock_);
        server_list.push_back(session);
        CreateIoCompletionPort((HANDLE)sock_, iocpHandle_, (ULONG_PTR)session, 0);
    }
}

IocpClient::~IocpClient(){
    Cleanup();
}


bool IocpClient::Start() {
    
    // if (!CreateListenSocket()) return false;
    
    // 워커 쓰레드 생성
    for (int i = 0; i < WORKER_COUNT; ++i) {
        workerThreads_.emplace_back([this]() { WorkerThread(); });
    }
    // 클라이언트 accept 처리 루프
    // workerThreads_.emplace_back([this]() { AcceptLoop(); });
    return true;
}

bool IocpClient::Connect(SERVER server , std::string ip , int port){ // 연결하고 싶은 서버 , ip , port
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET; //IPv4
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str()); //서버 주소

    LPFN_CONNECTEX pConnectEx = nullptr;
    GUID guidConnectEx = WSAID_CONNECTEX;
    DWORD bytes = 0;

    WSAIoctl(server_list[server]->GetSocket(), SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx),
            &pConnectEx, sizeof(pConnectEx), &bytes, nullptr, nullptr);

        
    //서버로 연결
    // int ret = WSAConnect(
    //     server_list[server]->GetSocket(),
    //     (sockaddr*)&serverAddr,
    //     sizeof(serverAddr),
    //     nullptr, nullptr, nullptr, nullptr
    // );
    
    // if (ret == SOCKET_ERROR) {
    //     int err = WSAGetLastError();
    //     // 비동기 연결일 경우 특정 에러는 무시해도 됨
    //     if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
    //         std::cerr << "서버 연결 실패: " << err << "\n";
    //         return false;
    //     }
    // }
    
    server_list[server]->Receive();     //서버 메세지 수신 준비
    servers_.insert(server_list[server]); //실제로 연결된 서버에 등록

    std::cout << "서버에 연결됨.\n";
    return true;
}



ServerSession * IocpClient::getServer(SERVER server){
    return server_list[server];
}