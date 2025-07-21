/*
    게임 내에서 사용할 객체 모음
    서로 다른 크기를 가질 수 있고 식별자에 대한 추가 정보를 입력받아야 하기 때문에
    GameObjectWrapper를 사용해 보완
*/
#include <unordered_map>
#include <memory> // unique_ptr를 위해 추가
#include "baseClass.h"

class GameObjectRegistry;
class GameObject;

// 오브젝트에 필요한 가상함수들을 정의
#define OBJECT_IDENTIFICATION(inCode,inClass)\
    static constexpr uint32_t objectId = inCode;\
    uint32_t GetObjectId() const override {return inCode;}\
    GameObject * CreateInstance() override {return new inClass();}\
    std::unique_ptr<GameObject> clone() const override { return std::make_unique<inClass>(*this); }\
    void update(const GameObject& source) override {\
        if (GetObjectId() != source.GetObjectId()) { return; }\
        *this = static_cast<const inClass&>(source);\
    }
    


#define OBJECT_REGIST(inClass)\
    {\
        GameObject * obj = new inClass();\
        registry[obj->GetObjectId()] = obj;\
    }


class GameObject : public BaseClass{
public:

    virtual uint32_t GetObjectId() const = 0;
    virtual GameObject * CreateInstance() = 0;
    virtual std::unique_ptr<GameObject> clone() const = 0; // clone 함수 추가
    virtual void update(const GameObject& source) = 0; // update 함수 추가 

};



class SampleObject : public GameObject{
public:
    int32_t hp;
    int32_t mp;
    float x;
    float y;
    float z;

    OBJECT_IDENTIFICATION(202020 , SampleObject)

    REFLECTABLE(SampleObject,
        MemberVariable("hp" , Type::Int32 , OffsetOf(SampleObject , hp)),
        MemberVariable("mp" , Type::Int32 , OffsetOf(SampleObject , mp)),
        MemberVariable("x" , Type::Float , OffsetOf(SampleObject , x)),
        MemberVariable("y" , Type::Float , OffsetOf(SampleObject , y)),
        MemberVariable("z" , Type::Float , OffsetOf(SampleObject , z))
    )

};

class SampleObject2 : public GameObject{
public:
    float x;
    float y;
    float z;

    OBJECT_IDENTIFICATION(202021 , SampleObject2)

    REFLECTABLE(SampleObject2,
        MemberVariable("x" , Type::Float , OffsetOf(SampleObject2 , x)),
        MemberVariable("y" , Type::Float , OffsetOf(SampleObject2 , y)),
        MemberVariable("z" , Type::Float , OffsetOf(SampleObject2 , z))
    )

};

class Chat : public GameObject{
public:
    uint32_t sessionId;
    std::string chat;

    OBJECT_IDENTIFICATION(202022 , Chat)

    REFLECTABLE(Chat,
        MemberVariable("sessionId" , Type::Int32 , OffsetOf(Chat , sessionId)),
        MemberVariable("chat" , Type::String , OffsetOf(Chat , chat)),
    )
};






// 게임 오브젝트의 생성을 담당
class GameObjectRegistry{
public:
    // 싱글톤 패턴
    static GameObjectRegistry * Get(){
        static GameObjectRegistry instance;
        return &instance;
    }

    GameObject * GetGameObject(uint32_t objectId){
        return registry[objectId]->CreateInstance();
    }

    
    // 게임 오브젝트를 등록
    void RegistGameObject(GameObject * obj){
        registry[obj->GetObjectId()] = obj;
    }

private:
    // 사용할 게임 오브젝트들을 등록
    GameObjectRegistry(){
        OBJECT_REGIST(SampleObject)
        OBJECT_REGIST(SampleObject2)
        OBJECT_REGIST(Chat)
    }
    std::unordered_map<uint32_t , GameObject *> registry;

};


// 오브젝트의 추가 정보를 기록하기 위한 인터페이스
class GameObjectWrapper : public BaseClass{
public:
    uint32_t networkId;
    uint32_t objectId;
    GameObject * obj;

    REFLECTABLE(GameObjectWrapper,
        MemberVariable("networkId" , Type::Int32 , OffsetOf(GameObjectWrapper , networkId)),
        MemberVariable("objectId" , Type::Int32 , OffsetOf(GameObjectWrapper , objectId)),
        MemberVariable("obj" , Type::Class , OffsetOf(GameObjectWrapper , obj) , Type::Class, [this](BaseClass * cls){
            /* 
            반환 함수는 static 함수이기 때문에 처음 호출될 때 값이 고정됨. 따라서 동적으로 객체를 반환하기 위해
            현재 직렬화 되고있는 Wrapper의 주소를 받아 직전에 직렬화 된 objectId를 활용해 레지스트리의 CreateInstance 함수를 실행한다.
            */
            return GameObjectRegistry::Get()->GetGameObject(((GameObjectWrapper *)cls)->objectId);
        }),
    )
};