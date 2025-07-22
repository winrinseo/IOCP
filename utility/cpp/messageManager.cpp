#include "messageManager.h"


MessageManager::MessageManager() : inMemoryStreamPool(10) , outMemoryStreamPool(10){
    gameObjectManager = GameObjectManager::Get();
    registRock = false;
}

MessageManager::MessageManager(uint32_t pool_size) : 
    inMemoryStreamPool(pool_size), 
    outMemoryStreamPool(pool_size){

    gameObjectManager = GameObjectManager::Get();
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

void MessageManager::SetSendGroup(std::function<void(uint32_t , char* , int)> f){
    sendGroup = f;
}

void MessageManager::ReplicationRun(uint32_t thread_count){

    for(int i = 0;i<thread_count;i++)
        runThread.emplace_back([this](){ replicationThread(); });
}


void MessageManager::replicationThread(){
    while(1){
        uint32_t networkGroup;
        // 없으면 블록
        auto replication_data = gameObjectManager->PopReplication(&networkGroup);
        
        std::unique_ptr<ReplicationData> data = std::make_unique<ReplicationData>();

        data->sessionId = 0;
        data->networkGroup = networkGroup;

        while(!replication_data.empty()){
            
            auto data_piece = std::move(replication_data.front());
            replication_data.pop();

            GameObjectWrapper * wrapper = new GameObjectWrapper();
            wrapper->networkId = data_piece.first;
            wrapper->objectId = data_piece.second->GetObjectId();

            // 만약 삭제된 객체면
            if(gameObjectManager->ObjectToAddress(wrapper->networkId) == nullptr)
                wrapper->objectId = 0; //objectId는 0으로 바꿔 삭제되었음을 알림
            
            wrapper->obj = data_piece.second.release(); // 메모리 누수 발생 지점 (일단 테스트 하고 개선)
            data->objList.push_back(wrapper);
        }

        // 직렬화
        char * buffer;
        int size;
        Serialize((BaseMessage*)data.get() , &buffer , &size );
        // 네트워크 그룹에 메세지 발신
        sendGroup(data->networkGroup , buffer , size);
    }
}