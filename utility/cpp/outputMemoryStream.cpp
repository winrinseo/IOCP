#include "outputMemoryStream.h"




void OutputMemoryStream::Write(void * data , uint32_t size){
    int resultHead = mHead + static_cast<uint32_t>(size);
    // 용량을 초과한다면 크기를 재할당함
    if(resultHead > mCapacity){
        ReallocBuffer(std::max(mCapacity * 2 ,resultHead));
    }
    
    // 버퍼의 다음 위치에 복사
    std::memcpy(buffer + mHead , data , size);
    mHead = resultHead;
}




void OutputMemoryStream::Serialize(void * data , uint32_t size){
    Write(data , size);
}

bool OutputMemoryStream::IsInput(){
    return false;
}