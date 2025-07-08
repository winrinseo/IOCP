/*
객체를 메모리로 출력하는 클래스
*/
#pragma once
#include "memoryStream.h"


class OutputMemoryStream : public MemoryStream{
public:

    void Write(void * data , uint32_t size);    

    void Serialize(void * data , uint32_t size) override;        // 기본 단위 직렬화

    bool IsInput() override;

private:
    
};