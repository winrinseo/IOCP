#pragma once
#include <winsock2.h>
#include <windows.h>

// 클라이언트와의 통신을 담당하는 세션 클래스
class ClientSession {
public:
    ClientSession(SOCKET socket);
    ~ClientSession();

    // 소켓 핸들 및 Overlapped 포인터 반환
    SOCKET GetSocket() const { return socket_; }
    OVERLAPPED* GetOverlapped() { return &overlapped_; }

    // 데이터 수신 요청
    void Receive();

    // 데이터 수신 완료 처리
    void OnReceive(DWORD bytesTransferred);

private:
    SOCKET socket_;                       // 클라이언트 소켓
    OVERLAPPED overlapped_;              // 비동기 I/O를 위한 구조체
    WSABUF wsaBuf_;                      // 데이터 버퍼 구조체
    char buffer_[1024];                  // 실제 데이터 버퍼
};
