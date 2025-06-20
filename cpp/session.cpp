#include "session.h"
#include <iostream>

Session::Session(SOCKET sock)
    : socket_(sock) {}

Session::~Session() {
    closesocket(socket_);
}

// 클라이언트로부터 데이터를 비동기로 수신 요청
void Session::Receive() {
    DWORD flags = 0;

    //메세지를 받을 공간 할당
    IoContext * context = new IoContext(); // Worker 스레드에서 처리하고 메모리 해제해야함
    context->operation = OperationType::RECV; // 수신으로 설정

    //버퍼 연결결
    context->wsaBuf.buf = context->buffer;
    context->wsaBuf.len = sizeof(context->buffer);

    std::cout<<this<<" "<<"받을 준비"<<"\n";

    //논블로킹 소켓 함수
    int ret = WSARecv(socket_, &context->wsaBuf, 1, nullptr, &flags, &context->overlapped, nullptr);
    if (ret == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            std::cerr << "WSARecv 실패: " << err << "\n";
            delete context; // 실패했으니 해제 필요
            return;
        }
    }
    //메세지 받고 큐에 삽입. 완료되었다고 통보
}



bool Session::Send(const char * data , int len){
    IoContext* context = new IoContext();
    context->operation = OperationType::SEND; //미리 설정 해두고 클라이언트에 송신 후 iocp에 통보

    // 보낼 데이터를 복사
    memcpy(context->buffer, data, len);
    context->wsaBuf.buf = context->buffer;
    context->wsaBuf.len = len;

    DWORD bytesSent = 0;
    int result = WSASend(socket_, &context->wsaBuf, 1, &bytesSent, 0, &context->overlapped, nullptr);
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING) {
            std::cerr << "WSASend 실패: " << err << std::endl;
            delete context;
            return false;
        }
    }
    //WSAsend하고 메모리 해제하면 안됨!! 큐에 완료 통보가 가는데 해당 완료 통보를 처리하는 것에 사용된다.

    return true;
}


