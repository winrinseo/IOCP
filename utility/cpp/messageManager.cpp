#include "messageManager.h"


MessageManager::MessageManager() : inMemoryStreamPool(10) , outMemoryStreamPool(10){
    registRock = false;
}

MessageManager::MessageManager(uint32_t pool_size) : 
    inMemoryStreamPool(pool_size), 
    outMemoryStreamPool(pool_size){

    registRock = false;
}

MessageManager::~MessageManager(){
    for(auto & pair : registry)
        delete pair.second;
    registry.clear();
    RPC.clear();
}

void MessageManager::RegistRock(){
    registRock = true;
}

void MessageManager::RegistUnrock(){
    registRock = false;
}

void MessageManager::regist(BaseMessage * reg , std::function<void(BaseMessage *)> f){
    if(registRock) return;
    registry[reg->GetId()] = reg;
    RPC[reg->GetId()] = f;
}

//원격 프로시저 호출
void MessageManager::CallRPC(BaseMessage * msg){
    if(RPC.find(msg->GetId()) != RPC.end())
        RPC[msg->GetId()](msg);
}

// 받은 정보를 등록된 메세지로 변환해 반환한다.
BaseMessage * MessageManager::Dispatch(const char* buffer, uint32_t size) {

    // 객체 획득 (ObjectPool 클래스 내부에서 동기화 처리 되어있기 때문에 바로 가져와도 됨)
    ObjectPool<InputMemoryStream>::ObjectPtr 
        inMemoryStream = inMemoryStreamPool.get();
        
    inMemoryStream->Prepare((char *)buffer , size);
    // 메세지 id 획득
    uint8_t mId = -1;
    inMemoryStream->Serialize(&mId , sizeof(uint8_t));
    // 등록된 샘플 객체
    auto it = registry.find(mId);

    // 등록된 메세지가 없을 경우
    if(it == registry.end())
        return nullptr;

    BaseMessage * msg = it->second->Create();

    // 메세지 역직렬화
    inMemoryStream->SerializeMessage(msg);

    return msg;

}


void MessageManager::Serialize(BaseMessage * msg , char ** output_buffer, int * output_length){

    ObjectPool<OutputMemoryStream>::ObjectPtr 
        outMemoryStream = outMemoryStreamPool.get();
    // 버퍼 준비
    outMemoryStream->Prepare();
    // 직렬화
    outMemoryStream->SerializeMessage(msg);

    // 직렬화 정보 반환
    *output_buffer = (char *) outMemoryStream->GetBuffer();
    *output_length = outMemoryStream->GetLength();
}