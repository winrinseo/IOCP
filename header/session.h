#pragma once
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <iostream>
#include <cstdint>


typedef enum _operationType {
    ACCEPT,
    CONNECT,
    RECV,
    SEND
} OperationType;

//OVERLAPPED 확장
struct IoContext {
    OVERLAPPED overlapped = {}; // 비동기 I/O를 위한 구조체
    OperationType operation; // 메세지 종류
    uint32_t connectId;     // AcceptEX시 소켓 아이디 식별용
    WSABUF wsaBuf = {}; // 데이터 버퍼 구조체
    char buffer[1024]; // 실제 데이터 버퍼
};


// 연결된 개체와의 통신을 담당하는 세션 클래스
class Session {
public:
    Session(SOCKET socket);
    ~Session();

    // 소켓 핸들 및 Overlapped 포인터 반환
    SOCKET GetSocket() const { return socket_; }


    //통신용 함수
    void Receive();                         // 데이터 수신 대기    
    bool Send(const char * data , int len); //해당 클라이언트에 메세지 송신
    

private:
    SOCKET socket_;                       // 클라이언트 소켓

};
