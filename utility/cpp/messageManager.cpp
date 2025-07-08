#include "messageManager.h"


MessageManager::MessageManager(){
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
BaseMessage * MessageManager::Dispatch(const char* buffer, uint32_t size) const {

    InputMemoryStream * inMemoryStream = new InputMemoryStream();
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

    delete inMemoryStream;
    return msg;
}

