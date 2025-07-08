#pragma once
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <thread>
#include <unordered_set>
#include <string>
#include <functional>

#include "outputMemoryStream.h"
#include "inputMemoryStream.h"

#include "session.h"


// 세션은 연결된 개체가 무엇인지에 따라 구분해 부를 수 있어야함
typedef class Session ServerSession , ClientSession;

// IOCP 인터페이스
class Iocp {
public:
    Iocp(int port); // 서버용 생성자
    Iocp(); // 클라용 생성자
    ~Iocp();

    void SetReceiveProcess(
        std::function<void(Session * session , const char * buffer , DWORD bytesTransferred)> f);   // 수신 완료 작업 설정
    void SetSendProcess(
        std::function<void()> f);       // 송신 완료 작업 설정

        

protected:
    std::string ip_;                                     // 연결 요청 IP
    int port_;                                          // 사용할 포트 번호
    SOCKET listenSocket_;                               // 리슨 소켓
    HANDLE iocpHandle_;                                 // IOCP 핸들
    std::vector<std::thread> workerThreads_;            // 워커 쓰레드 목록
    std::unordered_set<Session*> connects_;     // 연결된 개체 목록

    LPFN_ACCEPTEX pAcceptEx;                            //AcceptEx 함수 포인터

    std::function<void(Session * session , const char * buffer , DWORD bytesTransferred)> ReceiveProcess; // 수신 완료 시 수행 할 작업

    std::function<void()> SendProcess;                                                // 송신 완료 시 수행 할 작업

    static const int WORKER_COUNT = 4;                  // 워커 쓰레드 개수

    // 내부 동작 함수들
    virtual bool InitWinsock();                   // WinSock 초기화
    virtual bool CreateListenSocket();            // 리슨 소켓 생성
    virtual bool CreateIocp();                    // IOCP 생성
    virtual bool Start();                         // 시작
    virtual void PostAccept();                    // 클라이언트 Accept 비동기 예약
    virtual void AcceptLoop();                    // 클라이언트 Accept 루프
    virtual void WorkerThread();                  // 워커 쓰레드 함수
    virtual void Cleanup();                       // 정리 작업


private:
    void OnReceiveCompletion(Session * session , const char * buffer , DWORD bytesTransferred); // 데이터 수신 완료 처리
    void OnSendCompletion();    // 데이터 송신 완료 처리
};
