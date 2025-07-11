#include "iocpClient.h"


IocpClient::IocpClient(){}
IocpClient::IocpClient(uint32_t thread_size) : Iocp(thread_size){}

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

    // ConnectEx 함수 포인터 가져오기 
    SOCKET dummySocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (dummySocket == INVALID_SOCKET) {
        std::cerr << "더미 소켓 생성 실패: " << WSAGetLastError() << "\n";
        return false;
    }

    DWORD bytes = 0;
    GUID guidConnectEx = WSAID_CONNECTEX;
    if (WSAIoctl(dummySocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidConnectEx, sizeof(guidConnectEx),
        &pConnectEx, sizeof(pConnectEx),
        &bytes, NULL, NULL) == SOCKET_ERROR) {
        std::cerr << "ConnectEx 함수 포인터 가져오기 실패: " << WSAGetLastError() << "\n";
        closesocket(dummySocket);
        return false;
    }
    closesocket(dummySocket);
    
    //서버 세션 객체 생성 (연결은 따로 해야댐)
    // for(int i = 0;i<SERVER::end;i++){
    //     SOCKET sock_ = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    //     ServerSession * session = new ServerSession(sock_);
    //     server_list.push_back(session);
    //     CreateIoCompletionPort((HANDLE)sock_, iocpHandle_, (ULONG_PTR)session, 0);
    //     std::cout<<session<<" ";
    //     std::cout<<"\n";
    // }

    // 연결할 서버 개수 만큼 빈공간을 만들어줌
    server_list.resize(SERVER::end);
    
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
    
    // 1. 소켓 생성 및 IOCP 등록
    SOCKET clientSock = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0,WSA_FLAG_OVERLAPPED);
    if (clientSock == INVALID_SOCKET) {
        std::cerr << "클라이언트 소켓 생성 실패: " << WSAGetLastError() << "\n";
        return false;
    }

    // 세션에 소켓 설정
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        connects_[server] = std::make_unique<ServerSession>(clientSock);
        server_list[server] = connects_[server].get();
    }

    // IOCP에 소켓 등록
    if (CreateIoCompletionPort((HANDLE)clientSock, iocpHandle_, (ULONG_PTR)server, 0) == NULL) {
        std::cerr << "IOCP 등록 실패: " << GetLastError() << "\n";
        closesocket(clientSock);
        return false;
    }

    sockaddr_in bindAddr = {};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = 0; // 아무 포트나 사용
    bindAddr.sin_addr.s_addr = INADDR_ANY;

    // 2. ConnectEx를 위한 주소 바인딩 (필수)
    if (bind(clientSock, (SOCKADDR*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR) {
        std::cerr << "bind 실패: " << WSAGetLastError() << "\n";
        closesocket(clientSock);
        return false;
    }

    // 3. 비동기 ConnectEx 호출
    IoContext* context = new IoContext();
    context->operation = OperationType::CONNECT;
    ZeroMemory(&context->overlapped, sizeof(context->overlapped));

    BOOL ret = pConnectEx(
        clientSock,
        (SOCKADDR*)&serverAddr,
        sizeof(serverAddr),
        NULL, // 보내고 싶은 초기 데이터가 있다면 여기에 버퍼 포인터
       0,    // 초기 데이터 크기
       NULL, // 전송된 바이트 수 (비동기에서는 사용 안함)
       &context->overlapped
   );

   if (!ret && WSAGetLastError() != ERROR_IO_PENDING) {
       std::cerr << "ConnectEx 실패: " << WSAGetLastError() << "\n";
       delete context;
       closesocket(clientSock);
       return false;
   }

   // 성공적으로 비동기 연결 요청. 실제 연결 완료는 WorkerThread에서 처리됨.
   return true;
}


ServerSession * IocpClient::getServer(SERVER server){
    return server_list[server];
}