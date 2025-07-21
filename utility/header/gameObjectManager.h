/*
    서버에 현재 생성된 모든 게임 오브젝트를 관리하는 객체
    게임 오브젝트의 변화를 감지하고 다른 클라이언트로 상태를 전파시킬 수 있도록 변화를 알릴 수 있다.
    여기서 사용될 객체는 필수적으로 GameObjectRegistry에 등록한 이후 사용될 수 있다.
*/

#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include "baseMessage.h"



class GameObjectManager {
public:
    /* 싱글톤 패턴 */
    static GameObjectManager * Get(){
        static GameObjectManager instance;
        return &instance;
    }

    /* 오브젝트에 변화를 주는 함수 ( 실행되면 상태 전파를 위해 대기열에 등록 ) */

    void CreateObject(uint32_t networkGroup , GameObject * obj);                       // 오브젝트 생성
    void UpdateObject(uint32_t networkGroup, uint32_t networkId, GameObject * obj);    // 오브젝트 업데이트
    void DeleteObject(uint32_t networkGroup, uint32_t networkId);                      // 오브젝트 제거

    /* 오브젝트 변화를 받아들이는 함수 ( 다른 클라이언트로부터 오브젝트 상태 변화를 받음 )*/

    void CreateObjectFromOther(uint32_t networkGroup ,uint32_t networkId, GameObject * obj);    // 오브젝트 생성
    void UpdateObjectFromOther(uint32_t networkGroup , uint32_t networkId,GameObject * obj);    // 오브젝트 업데이트
    void DeleteObjectFromOther(uint32_t networkGroup, uint32_t networkId);    // 오브젝트 제거

    /* 오브젝트 저장소에 대한 연산 */

    std::queue<std::pair<uint32_t, std::unique_ptr<GameObject>>> PopReplication(uint32_t * networkGroup);   // 리플리케이션 가능한 오브젝트 모음 반환
    GameObject * ObjectToAddress(uint32_t networkId);
    uint32_t PopUpdatedNetworkId();      // 업데이트 완료된 오브젝트 id를 반환

private:
    GameObjectManager();

    uint32_t networkIdCounter;     // 부여할 네트워크 Id;

    std::mutex idMutex;     // id 동기화용 뮤텍스

    std::mutex updatedQueueMutex;
    std::mutex replicationQueueMutex; // 리플리케이션 큐 전체의 순서 제어

    std::condition_variable updatedQueue_cv; // 업데이트 큐 연산에 대한 상태변수
    std::condition_variable replicationQueue_cv; // 리플리케이션 큐 연산에 대한 상태변수
    
    std::unordered_map<uint32_t, std::mutex> groupMutex; // 네트워크 그룹별 동기화를 위한 뮤텍스

    std::unordered_map<uint32_t, // networkGroup
        std::unordered_map<uint32_t, std::unique_ptr<GameObject>>> actingObject; // 현재 서버에 생성된 오브젝트

    
    std::unordered_map<uint32_t , GameObject*> objectToAddress;       // 생성된 오브젝트의 주소를 빠르게 참조

    std::unordered_map<
        uint32_t, // networkGroup
        std::queue<
            std::pair<uint32_t, std::unique_ptr<GameObject>
        >>> replicableQueue; // 리플리케이션 가능한 오브젝트


    std::queue<uint32_t> updatedObject;         // 업데이트 완료된 오브젝트

};