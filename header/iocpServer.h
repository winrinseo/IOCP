#pragma once

#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <thread>
#include <unordered_set>

#include "iocp.h"


// IOCP 기반 서버를 담당하는 클래스
class IocpServer{
public:
    IocpServer(int port);
    ~IocpServer();

    bool Start();                           // 서버 시작

    void SetReceiveProcess(std::function<void(const char * buffer , DWORD bytesTransferred)> f);   // 수신 완료 작업 설정
    void SetSendProcess(void (*f)());       // 송신 완료 작업 설정

private:
    int port_;                             // 사용할 포트 번호
    SOCKET listenSocket_;                 // 리슨 소켓
    HANDLE iocpHandle_;                   // IOCP 핸들
    std::vector<std::thread> workerThreads_; // 워커 쓰레드 목록
    std::unordered_set<ClientSession*> clients_; // 접속한 클라이언트 목록

    std::function<void(const char * buffer , DWORD bytesTransferred)> ReceiveProcess;             // 수신 완료 시 수행 할 작업

    static const int WORKER_COUNT = 4;    // 워커 쓰레드 개수

    // 내부 동작 함수들
    bool InitWinsock();                   // WinSock 초기화
    bool CreateListenSocket();            // 리슨 소켓 생성
    bool CreateIocp();                    // IOCP 생성
    void AcceptLoop();                    // 클라이언트 Accept 루프
    void WorkerThread();                  // 워커 쓰레드 함수
    void Cleanup();                       // 정리 작업

    // 데이터 수신 완료 처리
    void OnReceiveCompletion(const char * buffer , DWORD bytesTransferred);


    // 데이터 송신 시 처리
    void OnSendCompletion();
};
