#pragma once
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <thread>
#include <unordered_set>
#include <string>
#include <functional>
#include <mutex>

#include "messageManager.h"

#include "session.h"

#define LISTENSOCKET_ID 987654321


// 기본 세션 아이디디
typedef enum _primary_id {
    GAME, 
    VOICE, 
    CHAT, 
    end
} SERVER;

// 세션은 연결된 개체가 무엇인지에 따라 구분해 부를 수 있어야함
typedef class Session ServerSession , ClientSession;

// IOCP 인터페이스
class Iocp {
public:
    Iocp(uint16_t port); // 서버용 생성자
    Iocp(uint16_t port , uint32_t thread_size); 
    Iocp(uint32_t thread_size); 
    Iocp(); // 클라용 생성자
    ~Iocp();

    virtual bool Start();                         // 시작
    virtual void Cleanup();                       // 정리 작업

    uint32_t GetSessionId();

    void SetReceiveProcess(
        std::function<void(uint32_t & sessionId , const char * buffer , DWORD bytesTransferred)> f);   // 수신 완료 작업 설정
    void SetSendProcess(
        std::function<void()> f);       // 송신 완료 작업 설정

    void RpcRegist(BaseMessage* reg , std::function<void(BaseMessage*)> f); // 원격 호출 함수 등록

    // 포인터 종류에 따른 인터페이스
    bool Send(const uint32_t & session_id , BaseMessage * msg);  // 해당 세션에 메세지 송신
    bool Send(const uint32_t & session_id , std::shared_ptr<BaseMessage> msg);  // 해당 세션에 메세지 송신
    bool Send(const uint32_t & session_id , std::unique_ptr<BaseMessage> msg);  // 해당 세션에 메세지 송신

protected:
    std::string ip_;                                    // 연결 요청 IP
    int port_;                                          // 사용할 포트 번호
    SOCKET listenSocket_;                               // 리슨 소켓
    HANDLE iocpHandle_;                                 // IOCP 핸들
    std::vector<std::thread> workerThreads_;            // 워커 쓰레드 목록

    std::mutex connectMutex;                            // 클라이언트 id 부여의 동기화
    std::mutex sessionsMutex;                          // 세션 목록 동기화
    uint32_t connectId;                                 // 생성된 소켓 수 (클라이언트 id 부여에 사용)
    uint32_t connectCount;                              // 연결된 객체 수
    std::unordered_map<uint32_t, std::unique_ptr<Session>> connects_;   // 연결된 개체 목록

    LPFN_ACCEPTEX pAcceptEx;                            //AcceptEx 함수 포인터
    LPFN_CONNECTEX pConnectEx;                          //ConnectEx 함수 포인터

    std::function<void(uint32_t & sessionId , const char * buffer , DWORD bytesTransferred)> 
                            ReceiveProcess;              // 수신 완료 시 수행 할 작업

    std::function<void()> SendProcess;                   // 송신 완료 시 수행 할 작업

    MessageManager messageManager;                      // 메세지 매니저 ( 수신된 메세지에 따른 동작 지정)

    bool _thread;
    bool _accept;

    int WORKER_COUNT;                  // 워커 쓰레드 개수

    // 내부 동작 함수들
    virtual bool InitWinsock();                   // WinSock 초기화
    virtual bool CreateListenSocket();            // 리슨 소켓 생성
    virtual bool CreateIocp();                    // IOCP 생성
    virtual void SetMessageManager();             // 메세지 매니저 세팅
    virtual void PostAccept();                    // 클라이언트 Accept 비동기 예약
    virtual void AcceptLoop();                    // 클라이언트 Accept 루프
    virtual void WorkerThread();                  // 워커 쓰레드 함수
    


private:

    void setPrimaryProceser();
    void OnReceiveCompletion(uint32_t & sessionId , const char * buffer , DWORD bytesTransferred); // 데이터 수신 완료 처리
    void OnSendCompletion();    // 데이터 송신 완료 처리
    

    uint32_t sessionId;

    std::unordered_map<uint32_t , std::vector<uint32_t>> networkGroup;
};
