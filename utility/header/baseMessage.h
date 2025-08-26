/*
여러 클래스 혹은 변수들을 하나의 메세지로 만들어 직렬화시키기 위함
*/
#pragma once
#include "baseClass.h"
#include "gameObject.h"

#define MESSAGE_IDENTIFICATION(inCode , inClass)\
    static constexpr uint8_t mId = inCode;\
    int GetId() const {return mId;}\
    BaseMessage * Create() const { return new inClass(); }


class BaseMessage : public BaseClass{
public:
    
    virtual int GetId() const = 0;
    virtual BaseMessage * Create() const = 0;
};

/* 필수 메세지 */
class Hello : public BaseMessage{
public:
    uint32_t sessionId;

    MESSAGE_IDENTIFICATION(1,Hello)
    REFLECTABLE(Hello,
        MemberVariable("sessionId", Type::Int32, OffsetOf(Hello, sessionId)),
    )

};


class IntoNetworkGroup : public BaseMessage{
public:
    uint32_t sessionId;
    uint32_t networkGroup;

    MESSAGE_IDENTIFICATION(2,IntoNetworkGroup)
    REFLECTABLE(IntoNetworkGroup,
        MemberVariable("sessionId", Type::Int32, OffsetOf(IntoNetworkGroup, sessionId)),
        MemberVariable("networkGroup", Type::Int32, OffsetOf(IntoNetworkGroup, networkGroup))
    )

};

class ReplicationData : public BaseMessage{
public:
    uint32_t sessionId;
    uint32_t networkGroup;
    std::vector<GameObjectWrapper*> objList;

    MESSAGE_IDENTIFICATION(3,ReplicationData)
    REFLECTABLE(ReplicationData,
        MemberVariable("sessionId", Type::Int32, OffsetOf(ReplicationData, sessionId)),
        MemberVariable("networkGroup", Type::Int32, OffsetOf(ReplicationData, networkGroup)),
        MemberVariable("objList", Type::Vector, OffsetOf(ReplicationData, objList) , Type::Class , [this](BaseClass* cls){
            return new GameObjectWrapper();
        }),
    )

};

// 객체 생성을 요청하는 메세지
class CreateObjectOrder : public BaseMessage{
public:
    uint32_t sessionId;
    uint32_t networkGroup;
    
    GameObjectWrapper * wrapper;

    MESSAGE_IDENTIFICATION(4,CreateObjectOrder)

    REFLECTABLE(CreateObjectOrder,
        MemberVariable("sessionId", Type::Int32, OffsetOf(CreateObjectOrder, sessionId)),
        MemberVariable("networkGroup", Type::Int32, OffsetOf(CreateObjectOrder, networkGroup)),
        MemberVariable("wrapper", Type::Class, OffsetOf(CreateObjectOrder, wrapper))
    )
};

// 객체 갱신을 요청하는 메세지
class UpdateObjectOrder : public BaseMessage{
public:
    uint32_t sessionId;
    uint32_t networkGroup;
    
    GameObjectWrapper * wrapper;

    MESSAGE_IDENTIFICATION(5,UpdateObjectOrder)

    REFLECTABLE(UpdateObjectOrder,
        MemberVariable("sessionId", Type::Int32, OffsetOf(UpdateObjectOrder, sessionId)),
        MemberVariable("networkGroup", Type::Int32, OffsetOf(UpdateObjectOrder, networkGroup)),
        MemberVariable("wrapper", Type::Class, OffsetOf(UpdateObjectOrder, wrapper))
    )
};

// 객체 삭제을 요청하는 메세지
class DeleteObjectOrder : public BaseMessage{
public:
    uint32_t sessionId;
    uint32_t networkGroup;
    
    GameObjectWrapper * wrapper;

    MESSAGE_IDENTIFICATION(6,DeleteObjectOrder)

    REFLECTABLE(DeleteObjectOrder,
        MemberVariable("sessionId", Type::Int32, OffsetOf(DeleteObjectOrder, sessionId)),
        MemberVariable("networkGroup", Type::Int32, OffsetOf(DeleteObjectOrder, networkGroup)),
        MemberVariable("wrapper", Type::Class, OffsetOf(DeleteObjectOrder, wrapper))
    )
};


/* 필수 메세지 */

//Sample 메세지
class Command : public BaseMessage{
public:
/*
    실질적으로 클라이언트와 통신하기위한 기본 단위임.
    기본적으로 베이스 클래스를 상속받지만 클라이언트와 통신하기 위한 몇가지 함수가 추가되어있음.

*/
    uint32_t num;
    Player * me;
    std::vector<Player*> cmdDeck; // 클래스는 항상 포인터형으로 선언

    MESSAGE_IDENTIFICATION(20,Command)

    REFLECTABLE(Command,
        MemberVariable("num", Type::Int32, OffsetOf(Command, num)),
        // 클래스 포인터 타입일 경우에는 반환 함수를 꼭 추가해야함
        MemberVariable("me", Type::Class, OffsetOf(Command, me) , Type::Class , [](BaseClass * cls){return new Player();}),
        MemberVariable("cmdDeck", Type::Vector, OffsetOf(Command, cmdDeck) , Type::Class , [](BaseClass * cls){return new Player();})
    )
};

class ChatMessage : public BaseMessage{
public:
    uint32_t sessionId;
    uint32_t networkGroup;
    std::string chat;

    MESSAGE_IDENTIFICATION(21,ChatMessage)

    REFLECTABLE(ChatMessage,
        MemberVariable("sessionId", Type::Int32, OffsetOf(ChatMessage, sessionId)),
        // 클래스 포인터 타입일 경우에는 반환 함수를 꼭 추가해야함
        MemberVariable("networkGroup", Type::Int32, OffsetOf(ChatMessage, networkGroup)),
        MemberVariable("chat", Type::String, OffsetOf(ChatMessage, chat))
    )
};

