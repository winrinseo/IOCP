/*
여러 클래스 혹은 변수들을 하나의 메세지로 만들어 직렬화시키기 위함
*/
#pragma once
#include "baseClass.h"
class BaseMessage : public BaseClass{
public:
    
    virtual int GetId() const = 0;
    virtual BaseMessage * Create() const = 0;
};

//Sample 메세지
class Command : public BaseMessage{
public:
    static constexpr uint8_t mId = 1; // 스태틱은 offset이 튀어서 따로 처리해야함
    uint32_t num;
    std::vector<Player*> cmdDeck; // 클래스는 항상 포인터형으로 선언

    int GetId() const {return mId;}
    BaseMessage * Create() const { return new Command(); }
    REFLECTABLE(Command,
        MemberVariable("num", Type::Int32, OffsetOf(Command, num)),
        // 클래스 포인터 타입일 경우에는 반환 함수를 꼭 추가해야함
        MemberVariable("cmdDeck", Type::Vector, OffsetOf(Command, cmdDeck) , Type::Class , [](){return new Player();})
    )
};