#pragma once
#include <winsock2.h>
#include <windows.h>


enum class OperationType {
    RECV,
    SEND
};

struct IoContext {
    OVERLAPPED overlapped = {}; // 비동기 I/O를 위한 구조체
    WSABUF wsaBuf = {}; // 데이터 버퍼 구조체
    char buffer[1024] = {}; // 실제 데이터 버퍼
    OperationType operation; // 메세지 종류
};


// 클라이언트와의 통신을 담당하는 세션 클래스
// OVERRAPPED 구조체에서 기능 추가
class ClientSession {
public:
    ClientSession(SOCKET socket);
    ~ClientSession();

    // 소켓 핸들 및 Overlapped 포인터 반환
    SOCKET GetSocket() const { return socket_; }
    // OVERLAPPED* GetOverlapped() { return &overlapped_; }

    // 데이터 수신 요청
    void Receive();

    // 데이터 수신 완료 처리
    virtual void OnReceiveCompletion(const char * buffer , DWORD bytesTransferred);

    //해당 클라이언트에 메세지 송신
    bool Send(const char * data , int len);
    virtual void OnSendCompletion();

private:
    SOCKET socket_;                       // 클라이언트 소켓
};
