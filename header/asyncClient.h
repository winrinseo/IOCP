#pragma once

#include <winsock2.h>
#include <windows.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// I/O 작업 정보를 담는 구조체
struct IoContext {
    OVERLAPPED overlapped = {};     // 비동기 작업 상태 추적용
    WSABUF wsaBuf = {};             // 송수신 버퍼 구조체
    char buffer[1024] = {};         // 실제 데이터 저장 공간
};

// 비동기 클라이언트 클래스
class AsyncClient {
public:
    AsyncClient(const std::string& ip, int port);  // 생성자: 서버 IP와 포트 초기화
    ~AsyncClient();                                // 소멸자: 소켓 정리 및 Winsock 종료

    bool Connect();                                // 서버에 연결 시도
    bool SendToServer(const std::string& message);  // 서버에 메시지 비동기 전송
    bool ReceiveMessage();                         // 서버로부터 메시지 비동기 수신

private:
    SOCKET sock_;           // 소켓 핸들
    std::string serverIp_;  // 서버 IP 주소
    int serverPort_;        // 서버 포트 번호
};
