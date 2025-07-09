#include "iocpClient.h"


IocpClient::IocpClient(){}

IocpClient::~IocpClient(){
    Cleanup();
}


bool IocpClient::Start() {
    if (!InitWinsock()) std::cerr << "WSAStartup 실패\n";
    if (!CreateIocp()) std::cout<<"iocp 실패"<<"\n";

    // 서버 동작 중엔 RPC 등록 불가능
    messageManager.RegistRock();

    _thread = true;
    _accept = true;
    
    //서버 세션 객체 생성 (연결은 따로 해야댐)
    for(int i = 0;i<SERVER::end;i++){
        SOCKET sock_ = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
        ServerSession * session = new ServerSession(sock_);
        server_list.push_back(session);
        CreateIoCompletionPort((HANDLE)sock_, iocpHandle_, (ULONG_PTR)session, 0);
        std::cout<<session<<" ";
        std::cout<<"\n";
    }
    
    // 워커 쓰레드 생성
    for (int i = 0; i < WORKER_COUNT; ++i) {
        workerThreads_.emplace_back([this]() { WorkerThread(); });
    }
    
    return true;
}


// 서버 연결
bool IocpClient::Connect(SERVER server , std::string ip , int port){ // 연결하고 싶은 서버 , ip , port
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET; //IPv4
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str()); //서버 주소

    //서버로 연결
    int ret = connect(
        server_list[server]->GetSocket(),
        (sockaddr*)&serverAddr,
        sizeof(serverAddr)
    );
    
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        // 비동기 연결일 경우 특정 에러는 무시해도 됨
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
            std::cerr << "서버 연결 실패: " << err << "\n";
            return false;
        }
    }

    connects_[GAME] = std::make_unique<ServerSession>(server_list[server]->GetSocket()); //실제로 연결된 서버에 등록
    server_list[server]->Receive();     //서버 메세지 수신 준비

    std::cout << "서버에 연결됨.\n";
    return true;
}



ServerSession * IocpClient::getServer(SERVER server){
    return server_list[server];
}