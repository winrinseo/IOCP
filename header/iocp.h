#pragma once
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <thread>
#include <unordered_set>
#include <string>
#include <functional>

#include "session.h"

// 세션은 연결된 개체가 무엇인지에 따라 구분해 부를 수 있어야함
typedef class Session ServerSession , ClientSession;

// IOCP 인터페이스
class Iocp {
public:
    // IOCP 시작
    bool Start();

private:
    std::string ip;                         // 연결 요청 IP
    int port_;                             // 사용할 포트 번호
    SOCKET listenSocket_;                 // 리슨 소켓
    HANDLE iocpHandle_;                   // IOCP 핸들
    std::vector<std::thread> workerThreads_; // 워커 쓰레드 목록
    std::unordered_set<Session*> connect_; // 접속한 개체 목록

    static const int WORKER_COUNT = 4;    // 워커 쓰레드 개수

    // 내부 동작 함수들
    virtual bool InitWinsock();                   // WinSock 초기화
    virtual bool CreateListenSocket();            // 리슨 소켓 생성
    virtual bool CreateIocp();                    // IOCP 생성
    virtual void AcceptLoop();                    // 클라이언트 Accept 루프
    virtual void WorkerThread();                  // 워커 쓰레드 함수
    virtual void Cleanup();                       // 정리 작업
};
