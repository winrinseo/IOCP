/*
직렬화 , 역직렬화를 위한 메모리 스트림
*/
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <vector>

#include "baseMessage.h"


// 벡터 타입 매핑 구조체
template<Type T>
struct TypeMap;

// 템플릿 특수화
template<>
struct TypeMap<Type::Int8> { using type = uint8_t; };

template<>
struct TypeMap<Type::Int16> { using type = uint16_t; };

template<>
struct TypeMap<Type::Int32> { using type = uint32_t; };

template<>
struct TypeMap<Type::Float> { using type = float; };

template<>
struct TypeMap<Type::String> { using type = std::string; };



class MemoryStream{
public:

    void Prepare();                                             // 메모리 준비(output)
    void Prepare(char * buffer , uint32_t size);                // 메모리 준비 (input)
    void MemoryFree();                                          // 할당된 메모리 제거

    const char * GetBuffer() { return buffer;};
    int GetLength() {return mHead; }

    template<typename T>
    void SerializeVector(std::vector<T>& inVector);
    void SerializeString(std::string& str);

    template<Type E>
    void DispatchVectorSerialization(void* data);

    void SerializeMessage(BaseMessage * data);                         // 준비된 메세지를 직렬화 or 역직렬화
    void Serialize(BaseClass * data);                           // 클래스 하나를 직렬화 or 역직렬화

    virtual void Serialize(void * data , uint32_t inByteCount) = 0;  // 변수 하나를 직렬화 or 역직렬화
    virtual bool IsInput() = 0;                                 // 메모리 input or output

protected:
    char * buffer;
    int mHead;
    int mCapacity;

    void ReallocBuffer(uint32_t inNewLength);                        // 메모리 재할당 (출력일때만 쓸듯)
};