#include "AsyncClient.h"
#include <iostream>

// 생성자: Winsock 초기화 및 비동기 소켓 생성
AsyncClient::AsyncClient(const std::string& ip, int port)
    : sock_(INVALID_SOCKET), serverIp_(ip), serverPort_(port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup 실패\n";
    }

    // 비동기용 소켓 생성 (WSA_FLAG_OVERLAPPED)
    sock_ = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (sock_ == INVALID_SOCKET) {
        std::cerr << "소켓 생성 실패\n";
    }
}

// 소멸자: 소켓 및 Winsock 해제
AsyncClient::~AsyncClient() {
    if (sock_ != INVALID_SOCKET) {
        closesocket(sock_);
    }
    WSACleanup();
}

// 서버에 연결 시도
bool AsyncClient::Connect() {
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET; //IPv4
    serverAddr.sin_port = htons(serverPort_);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp_.c_str()); //서버 주소

    int ret = connect(sock_, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        // 비동기 연결일 경우 특정 에러는 무시해도 됨
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
            std::cerr << "서버 연결 실패: " << err << "\n";
            return false;
        }else{
            std::cerr << "서버 연결 실패: " << err << "\n";
            return false;
        }
    }

    std::cout << "서버에 연결됨.\n";
    return true;
}

// 메시지 비동기 전송
bool AsyncClient::SendToServer(const std::string& message) {
    IoContext sendCtx;
    //버퍼에 메세지 복사
    memcpy(sendCtx.buffer, message.c_str(), message.size());

    sendCtx.wsaBuf.buf = sendCtx.buffer;
    sendCtx.wsaBuf.len = static_cast<ULONG>(message.size());

    DWORD bytesSent = 0;
    //메세지 송신, overlappedIO는 비동기 통신에 사용된다.
    int ret = WSASend(sock_, &sendCtx.wsaBuf, 1, &bytesSent, 0, &sendCtx.overlapped, nullptr);
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        std::cerr << "WSASend 실패\n";
        return false;
    }

    // 비동기 전송 성공 또는 대기 중
    return true;
}

// 메시지 비동기 수신
bool AsyncClient::ReceiveMessage() {
    IoContext recvCtx;
    recvCtx.wsaBuf.buf = recvCtx.buffer;
    recvCtx.wsaBuf.len = sizeof(recvCtx.buffer);

    DWORD flags = 0;
    DWORD received = 0;

    //메세지 수신
    int ret = WSARecv(sock_, &recvCtx.wsaBuf, 1, &received, &flags, &recvCtx.overlapped, nullptr);
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        std::cerr << "WSARecv 실패\n";
        return false;
    }

    // 결과 대기 (비동기지만 여기선 블로킹 처리)
    BOOL result = WSAGetOverlappedResult(sock_, &recvCtx.overlapped, &received, TRUE, &flags);
    if (!result || received == 0) {
        std::cout<<result<<" "<<received<<" \n";
        std::cerr << "서버 응답 수신 실패 또는 연결 종료됨\n";
        return false;
    }

    std::cout << "[서버 응답]: " << std::string(recvCtx.buffer, received) << "\n";
    return true;
}
