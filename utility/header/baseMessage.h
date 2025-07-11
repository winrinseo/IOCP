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
/*
    실질적으로 클라이언트와 통신하기위한 기본 단위임.
    기본적으로 베이스 클래스를 상속받지만 클라이언트와 통신하기 위한 몇가지 함수가 추가되어있음.

*/
    static constexpr uint8_t mId = 1; // 스태틱은 offset이 튀어서 따로 처리해야함
    uint32_t num;
    Player * me;
    std::vector<Player*> cmdDeck; // 클래스는 항상 포인터형으로 선언

    int GetId() const {return mId;}
    BaseMessage * Create() const { return new Command(); }
    REFLECTABLE(Command,
        MemberVariable("num", Type::Int32, OffsetOf(Command, num)),
        // 클래스 포인터 타입일 경우에는 반환 함수를 꼭 추가해야함
        MemberVariable("me", Type::Class, OffsetOf(Command, me) , Type::Class , [](){return new Player();}),
        MemberVariable("cmdDeck", Type::Vector, OffsetOf(Command, cmdDeck) , Type::Class , [](){return new Player();})
    )
};