#include "clientSession.h"
#include <iostream>

ClientSession::ClientSession(SOCKET sock)
    : socket_(sock) {
    ZeroMemory(&overlapped_, sizeof(overlapped_));
    wsaBuf_.buf = buffer_;
    wsaBuf_.len = sizeof(buffer_);
}

ClientSession::~ClientSession() {
    closesocket(socket_);
}

// 클라이언트로부터 데이터를 비동기로 수신 요청
void ClientSession::Receive() {
    DWORD flags = 0;
    //논블로킹 소켓 함수
    WSARecv(socket_, &wsaBuf_, 1, nullptr, &flags, &overlapped_, nullptr);
}

// 수신 완료 후 처리
void ClientSession::OnReceive(DWORD bytesTransferred) {

    //대충 여기서 수신 후 연산 수행
    {
        std::cout << "받은 데이터: " << std::string(buffer_, bytesTransferred) << std::endl;
        // 받은 데이터를 그대로 에코(되돌려 보냄)
        send(socket_, buffer_, bytesTransferred, 0);
    }


    // 다음 수신 준비
    ZeroMemory(&overlapped_, sizeof(overlapped_));
    Receive();
}
