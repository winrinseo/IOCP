#include "inputMemoryStream.h"


// 메모리 스트림을 읽어 data 포인터에 값을 넣음
void InputMemoryStream::Read(void * data , uint32_t size){
    int resultHead = mHead + static_cast<int>(size);

    // 헤드가 용량을 초과할 때
    if (resultHead > mCapacity) 
        throw std::runtime_error("Read overflow");
    

    memcpy(data , buffer + mHead , size);

    mHead = resultHead;
}


void InputMemoryStream::Serialize(void * data , uint32_t size){
    Read(data , size);
}

bool InputMemoryStream::IsInput(){
    return true;
}