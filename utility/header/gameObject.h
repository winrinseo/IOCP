/*
    게임 내에서 사용할 객체 모음
    서로 다른 크기를 가질 수 있고 식별자에 대한 추가 정보를 입력받아야 하기 때문에
    GameObjectWrapper를 사용해 보완
*/
#include <unordered_map>
#include "baseClass.h"

class GameObjectRegistry;
class GameObject;

#define OBJECT_IDENTIFICATION(inCode,inClass)\
    static constexpr uint32_t objectId = inCode;\
    uint32_t GetObjectId() override {return inCode;}\
    GameObject * CreateInstance() override {return new inClass();}
    


#define OBJECT_REGIST(inClass)\
    {\
        GameObject * obj = new inClass();\
        registry[obj->GetObjectId()] = obj;\
    }


class GameObject : public BaseClass{
public:

    virtual uint32_t GetObjectId() = 0;
    virtual GameObject * CreateInstance() = 0;

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






// 게임 오브젝트의 생성을 담당
class GameObjectRegistry{
public:
    static GameObjectRegistry * Get(){
        static GameObjectRegistry instance;
        return &instance;
    }

    GameObject * GetGameObject(uint32_t objectId){
        return registry[objectId]->CreateInstance();
    }

private:
    GameObjectRegistry(){
        OBJECT_REGIST(SampleObject)
        OBJECT_REGIST(SampleObject2)
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
            return GameObjectRegistry::Get()->GetGameObject(((GameObjectWrapper *)cls)->objectId);
        }),

    )
};