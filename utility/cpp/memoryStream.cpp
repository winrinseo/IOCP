#include "memoryStream.h"

// 벡터 템플릿 설정
template<Type E>
void MemoryStream::DispatchVectorSerialization(void* data , const MemberVariable& mv) {
    using T = typename TypeMap<E>::type;
    auto* vecPtr = reinterpret_cast<std::vector<T>*>(data);
    SerializeVector<T>(*vecPtr);
}

// 템플릿 특수화화
template<>
void MemoryStream::DispatchVectorSerialization<Type::Class>(void* data , const MemberVariable& mv){
    auto* vecPtr = reinterpret_cast<std::vector<BaseClass*>*>(data);
    // 벡터의 타입이 클래스 포인터형일 경우
    if(IsInput()){
        // 역직렬화에서는 정보를 담을 객체를 준비해주는 과정이 필요하다.
        uint8_t size = 0;
        Serialize(&size, sizeof(size));
        vecPtr->resize(size);
        // 각 원소에 객체를 생성해준다.
        for(int i = 0;i<size;i++)
            (*vecPtr)[i] = mv.CreateInstance(nullptr);
        
    }
    SerializeVector<BaseClass*>(*vecPtr);
}

// 직렬화 메모리 할당
void MemoryStream::Prepare(){
    // 이 객체가 직접 할당한 메모리만 직접 제거함
    if(this->buffer != nullptr)
        MemoryFree(); 
    this->buffer = nullptr;
    this->mCapacity = 0;
    this->mHead = 0;
    ReallocBuffer(32);
}

// 역직렬화할 메모리 할당
void MemoryStream::Prepare(char * buffer , uint32_t size){
    // 외부에서 할당된 메모리를 읽는 것이기 때문에 메모리를 제거하지 않는다.
    this->buffer = buffer;
    this->mCapacity = size;
    this->mHead = 0;
}

// 할당된 메모리 해제
void MemoryStream::MemoryFree(){
    std::free(this->buffer);
    this->buffer = nullptr;
    this->mCapacity = 0;
    this->mHead = -1;
}


// 메모리 재할당
void MemoryStream::ReallocBuffer(uint32_t inNewLength){
    buffer = static_cast<char *>(std::realloc(buffer , inNewLength));
    mCapacity = inNewLength;
}

// 벡터 직렬화 일반화
template<typename T>
void MemoryStream::SerializeVector(std::vector<T>& vec) {
    uint8_t size = vec.size();
    
    if(!IsInput()){
        size = static_cast<uint32_t>(vec.size());
        Serialize(&size, sizeof(size));
    }else if(size == 0) { // 클래스 포인터 타입의 경우 외부에서 사이즈를 결정해둔다.
        Serialize(&size, sizeof(size));
        vec.resize(size);
    }

    for (uint32_t i = 0; i < size; ++i) {
        if constexpr (std::is_same_v<T, std::string>) {
            // 문자열 벡터 처리
            SerializeString(vec[i]);
        }else if constexpr (std::is_pointer_v<T>
                         && std::is_base_of_v<BaseClass, std::remove_pointer_t<T>>) {
                            // 베이스 클래스를 상속받은 포인터 타입인지 확인
            // 사용자 정의 타입, 베이스 클래스 벡터는 원소로 주소값을 가지고있음
            Serialize(reinterpret_cast<BaseClass*>(vec[i])); // 개별 필드를 순회하는 메타 기반 함수
        }
        else {
            // POD 타입
            Serialize(&vec[i], sizeof(T));
        }
    }
}

// 문자열 직렬화
void MemoryStream::SerializeString(std::string& str) {
    uint8_t length;
    if (IsInput()) {
        Serialize(&length, sizeof(length));
        str.resize(length);
        Serialize(&str[0], length);  // 안전: string은 &str[0]이 가능
    } else {
        length = static_cast<uint8_t>(str.size());
        Serialize(&length, sizeof(length));
        Serialize(str.data(), length);
    }

}




// 클래스 하나를 직렬화 or 역직렬화
void MemoryStream::Serialize(BaseClass * data){
    for(auto& mv : data->GetDataType()){
        void * mvData = reinterpret_cast<char*>(data) + mv.GetOffset();
        switch(mv.GetType()){
            case Type::Int8:
                Serialize(mvData , sizeof(uint8_t));
                break;

            case Type::Int16:
                Serialize(mvData , sizeof(uint16_t));
                break;

            case Type::Int32:
                Serialize(mvData , sizeof(uint32_t));
                break;

            case Type::Float:
                Serialize(mvData , sizeof(float));
                break;
            
            case Type::String:
            {
                std::string* strPtr = static_cast<std::string*>(mvData);
                SerializeString(*strPtr);
                break;
            }
            case Type::Vector:
                
                switch (mv.GetElementType()){
                    case Type::Int8: DispatchVectorSerialization<Type::Int8>(mvData,mv); break;
                    case Type::Int16: DispatchVectorSerialization<Type::Int16>(mvData,mv); break;
                    case Type::Int32: DispatchVectorSerialization<Type::Int32>(mvData,mv); break;
                    case Type::String: DispatchVectorSerialization<Type::String>(mvData,mv); break;
                    case Type::Float: DispatchVectorSerialization<Type::Float>(mvData,mv); break;

                    default: // 사용자 정의 클래스
                        DispatchVectorSerialization<Type::Class>(mvData,mv);
                        break;
                    
                }
                
                break;
                
            
            default: // 사용자 정의 클래스
            {
                //포인터 변수 mvData도 포인터 변수를 가지기 때문에 역참조를 위해 더블포인터 사용
                BaseClass** classPtr = reinterpret_cast<BaseClass**>(mvData);
                // 역직렬화에서는 정보를 담을 객체를 준비해주는 과정이 필요하다.
                if(IsInput()){
                    // 타입이 클래스 포인터형일 경우 (GameObject의 경우 wrapper의 주소를 보내줘야함)
                    // CreateInstance가 static이라 처음 생성될 때 생성할 오브젝트가 정해져버림..
                    // 그래서 현재 wrapper 주소를 보내주고 여기서 objectId를 확인하도록 함
                    *classPtr = mv.CreateInstance(data);
                }
                Serialize(*classPtr);
                break;
            }
        }
    }
    
}

// 메세지 하나를 직렬화 or 역직렬화
void MemoryStream::SerializeMessage(BaseMessage * data){
    if(!IsInput()){
        // 출력일때만 메세지 Id를 메모리에 기록
        uint8_t mId = data->GetId();
        Serialize(&mId,sizeof(uint8_t));
        //입력일 때는 외부에서 먼저 읽어야됨
    }
    Serialize((BaseClass*)data);
}