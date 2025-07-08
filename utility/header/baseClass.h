/*
직렬화에 사용될 클래스 등을 정의
*/
#pragma once
#include <initializer_list>
#include <functional>
#include <vector>
#define OffsetOf(c,mv) ((size_t)& static_cast<c*>(nullptr)->mv)

#define REFLECTABLE(CLASS, ...) \
    inline static DataType* sDataType = nullptr; \
    const std::vector<MemberVariable>& GetDataType() const override { return sDataType->GetMemberVariables(); } \
    static void InitDataType() { \
        sDataType = new DataType({ __VA_ARGS__ }); \
    }

enum Type{
    Int8,
    Int16,
    Int32,
    String,
    Float,

    Vector,

    Class
};

class BaseClass;

// 클래스의 멤버 변수 타입
class MemberVariable {
public:
    MemberVariable(const char* inName, Type inType, uint32_t inOffset, 
        Type elementType = Type::Int32 , std::function<BaseClass*()> fac = nullptr) 
        : mName(inName), mType(inType), mOffset(inOffset), mElementType(elementType), mFactory(fac) {}

    Type GetType() const { return mType; }
    Type GetElementType() const { return mElementType; }
    uint32_t GetOffset() const { return mOffset; }
    BaseClass* CreateInstance() const { 
        // 클래스 용도일 땐 factory로, 아니면 nullptr
        return mFactory ? mFactory() : nullptr; 
    }

private:
    std::string mName;
    Type mType;
    uint32_t mOffset;

    Type mElementType;  // 템플릿 타입
    std::function<BaseClass*()> mFactory; // 재귀 반환 함수, 본인의 타입 객체를 반환함 (클래스 타입일 때만 사용)
};

// 멤버 변수 리스트
class DataType{
public:
    DataType(std::vector<MemberVariable> inMVs) :
    mMemberVariables(inMVs)
    {}

    const std::vector<MemberVariable>& GetMemberVariables() const{
        return mMemberVariables;
    }

private:
    std::vector<MemberVariable> mMemberVariables;
};




class BaseClass{
public:
    virtual const std::vector<MemberVariable>& GetDataType() const = 0;
};


// Sample 클래스
class Player : public BaseClass{
public:
    /*
    라이브러리에서 사용될 모든 클래스, 메세지는 BaseClass를 상속받아 아래처럼 
    REFLECTABLE 매크로를 추가해야 사용 가능함.
    만약 클래스나 메세지 내부에 직렬화할 클래스를 추가할 경우 반드시 포인터형으로 추가해야함.
    */
    uint32_t mId;
    std::string mName;
    std::vector<uint32_t> mScore;
    // 리플렉션에 필요한 것들을 매크로로 묶어 사용
    REFLECTABLE(Player,
        MemberVariable("mId", Type::Int32, OffsetOf(Player, mId)),
        MemberVariable("mName", Type::String, OffsetOf(Player, mName)),
        MemberVariable("mScore", Type::Vector, OffsetOf(Player, mScore) , Type::Int32) //템플릿 타입 명시
    )
};
