#pragma once
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <thread>
#include <unordered_set>

// #include "clientSession.h"

// #pragma comment(lib, "ws2_32.lib")

class ClientSession;

// IOCP 기반 서버를 담당하는 클래스
class Iocp {
public:
    Iocp(int port);
    ~Iocp();

    // 서버 시작
    bool Start();

private:
    int port_;                             // 사용할 포트 번호
    SOCKET listenSocket_;                 // 리슨 소켓
    HANDLE iocpHandle_;                   // IOCP 핸들
    std::vector<std::thread> workerThreads_; // 워커 쓰레드 목록
    std::unordered_set<ClientSession*> clients_; // 접속한 클라이언트 목록

    static const int WORKER_COUNT = 4;    // 워커 쓰레드 개수

    // 내부 동작 함수들
    bool InitWinsock();                   // WinSock 초기화
    bool CreateListenSocket();            // 리슨 소켓 생성
    bool CreateIocp();                    // IOCP 생성
    void AcceptLoop();                    // 클라이언트 Accept 루프
    void WorkerThread();                  // 워커 쓰레드 함수
    void Cleanup();                       // 정리 작업
};
