/*
    특정 메세지를 수신받았을 때의 동작을 정의할 수 있다.
    수동으로 역직렬화해 동작을 정의하는 것은 복잡하기 때문에,
    메세지 매니저를 통한 원격 프로시저 호출을 권장함.

*/

#pragma once
#include <unordered_map>
#include <thread>
#include "baseMessage.h"
#include "inputMemoryStream.h"
#include "outputMemoryStream.h"
#include "objectPool.h"
#include "gameObjectManager.h"

class MessageManager{

    
public:
    MessageManager();
    MessageManager(uint32_t pool_size);
    ~MessageManager();
    void RegistRock();
    void RegistUnrock();
    void regist(BaseMessage * reg , std::function<void(BaseMessage *)> f);  //원격 프로시저 등록
    void CallRPC(BaseMessage * msg);                                        //원격 프로시저 호출
    BaseMessage * Dispatch(const char* buffer, uint32_t size);              //등록된 메세지 중 일치하는 메세지 타입으로 변경

    void Serialize(BaseMessage * msg , char ** output_buffer, int * output_length); // 메세지 직렬화

    /* 리플리케이션 */
    void SetSendGroup(std::function<void(uint32_t , char* , int)> f);
    void ReplicationRun(uint32_t thread_count);


private:
    
    std::unordered_map<uint32_t , BaseMessage *> registry;  //메세지 등록

    //원격 메소드 호출 (특정 메세지가 들어오면 자동으로 실행될 함수)
    //서버에 저장된 정보를 바꾸기 위해선 서버 내부에서 public 함수를 작성해 등록해야한다.
    std::unordered_map<uint32_t , std::function<void(BaseMessage *)>> RPC;

    bool registRock;    // 서비스가 실행중일 때 등록 불가

    ObjectPool<InputMemoryStream> inMemoryStreamPool;           // 직렬화 오브젝트 풀
    ObjectPool<OutputMemoryStream> outMemoryStreamPool;         // 역직렬화 오브젝트 풀

    /* 리플리케이션 */
    GameObjectManager * gameObjectManager;
    std::function<void(uint32_t networkGroup, char * buffer , int size)> sendGroup;
    std::vector<std::thread> runThread;
    void replicationThread(); // 리플리케이션 가능한 오브젝트를 그룹에 속한 클라이언트에게 전파

};