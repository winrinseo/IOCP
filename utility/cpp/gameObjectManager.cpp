#include "gameObjectManager.h"
#include <iostream>

GameObjectManager::GameObjectManager(){}

// 네트워크 그룹에 오브젝트를 생성함
void GameObjectManager::CreateObject(uint32_t networkGroup , GameObject * obj){
    
    uint32_t net_id;
    {   
        std::lock_guard<std::mutex> lock(idMutex);
        net_id = ++networkIdCounter;
    }
    objectToAddress[net_id] = obj;
    actingObject[networkGroup][net_id] = std::unique_ptr<GameObject>(obj);

    // 리플리케이션 대기열에 등록
    // obj의 clone() 가상 함수를 호출하여 실제 타입에 맞는 복사본을 생성
    replicableQueue[networkGroup].push(std::make_pair(net_id , obj->clone()));
    replicationQueue_cv.notify_one();
}

void GameObjectManager::UpdateObject(uint32_t networkGroup, uint32_t networkId, GameObject * obj){
    std::lock_guard<std::mutex> lock(groupMutex[networkGroup]);

    // 기존 객체를 업데이트 ( objectId가 다르면 업데이트하지 않음 )
    actingObject[networkGroup][networkId]->update(*obj);

    // 리플리케이션 대기열에 등록
    // obj의 clone() 가상 함수를 호출하여 실제 타입에 맞는 복사본을 생성
    replicableQueue[networkGroup].push(std::make_pair(networkId , obj->clone()));
    replicationQueue_cv.notify_one();
}

// 오브젝트 삭제
void GameObjectManager::DeleteObject(uint32_t networkGroup, uint32_t networkId){
    std::lock_guard<std::mutex> lock(groupMutex[networkGroup]);

    replicableQueue[networkGroup].push(std::make_pair(networkId , actingObject[networkGroup][networkId]->clone()));
    replicationQueue_cv.notify_one();

    objectToAddress.erase(networkId);
    actingObject[networkGroup].erase(networkId);
}


// 다른 클라이언트(서버)에 의해 객체가 생성됨
void GameObjectManager::CreateObjectFromOther(uint32_t networkGroup , uint32_t networkId,GameObject * obj){
    
    { // 범위가 너무 넓으면 싱글 스레드와 다를게 없음
        std::lock_guard<std::mutex> lock(groupMutex[networkGroup]);
        objectToAddress[networkId] = obj;
        actingObject[networkGroup][networkId] = std::unique_ptr<GameObject>(obj);
    }

    // 업데이트 대기열에 등록
    {
        std::lock_guard<std::mutex> lock(updatedQueueMutex);
        updatedObject.push(networkId);
    }
    updatedQueue_cv.notify_one();
}

void GameObjectManager::UpdateObjectFromOther(uint32_t networkGroup , uint32_t networkId,GameObject * obj){
    {
        std::lock_guard<std::mutex> lock(groupMutex[networkGroup]);

        // 기존 객체를 업데이트 ( objectId가 다르면 업데이트하지 않음 )
        actingObject[networkGroup][networkId]->update(*obj);
    }

    // 업데이트 대기열에 등록
    {
        std::lock_guard<std::mutex> lock(updatedQueueMutex);
        updatedObject.push(networkId);
    }
    updatedQueue_cv.notify_one();
}

void GameObjectManager::DeleteObjectFromOther(uint32_t networkGroup, uint32_t networkId){
    {
        std::lock_guard<std::mutex> lock(groupMutex[networkGroup]);

        // 기존 객체를 업데이트 ( objectId가 다르면 업데이트하지 않음 )
        actingObject[networkGroup].erase(networkId);
    }

    // 업데이트 대기열에 등록
    {
        std::lock_guard<std::mutex> lock(updatedQueueMutex);
        updatedObject.push(networkId);
    }
    updatedQueue_cv.notify_one();
}


uint32_t GameObjectManager::PopUpdatedNetworkId(){
    std::unique_lock<std::mutex> lock(updatedQueueMutex);
    // 비어있다면 대기
    updatedQueue_cv.wait(lock, [this]{ return !updatedObject.empty();});
    uint32_t ret = updatedObject.front();
    updatedObject.pop();
    return ret;
}


// 임의의 네트워크 그룹 리플리케이션 큐를 반환
std::queue<std::pair<uint32_t, std::unique_ptr<GameObject>>> 
    GameObjectManager::PopReplication(uint32_t * networkGroup){
    
    std::unique_lock<std::mutex> replication_lock(replicationQueueMutex);
    replicationQueue_cv.wait(replication_lock , [this]{return !replicableQueue.empty();});
    auto it = replicableQueue.begin();
    
    
    // 사용중이라면 대기
    std::lock_guard<std::mutex> group_lock(groupMutex[it->first]);

    // replicableQueue의 networkGroup에 유일하게 접근하고 있는 상태
    *networkGroup = it->first;
    auto ret = std::move(it->second);

    // 그룹 삭제
    replicableQueue.erase(it);

    return ret;
}

GameObject * GameObjectManager::ObjectToAddress(uint32_t networkId){
    if(objectToAddress.find(networkId) == objectToAddress.end())
        return nullptr;
    return objectToAddress[networkId];
}