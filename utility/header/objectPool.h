/*
 멀티 스레드 환경에서 사용할 수 있게 동기화 처리 되어있는 함수를 제공

*/

#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <stdexcept>

template <typename T>
class ObjectPool{
public:
    virtual ~ObjectPool() = default;

    using ObjectPtr = std::unique_ptr<T , std::function<void(T * ptr)>>;

    ObjectPool(uint32_t size = 0);

    ObjectPtr get();

    void SetPreRelease(std::function<void(T*)> f) { preRelease = f; }


private:
    /* 
        복사 및 이동 생성자/대입 연산자 비활성화
        복사 및 이동을 허용하게 되면 스마트 포인터의 소유권 관련 문제로
        잠재적인 오류가 발생할 수 있음 (메모리 더블 프리 , 댕글링 포인터)
    */
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;                      

    void release(T * ptr);                      // 스마트 포인터 커스텀 제거자 (메모리를 해제하지 않고 풀에 반납)

    void addObject(uint32_t count);             // 객체 추가

    std::function<void(T *)> preRelease;        // 객체 회수 전에 필요한 동작을 정의
    std::mutex m_mutex;                         // 스레드 동기화
    std::condition_variable cv;                 // 대기를 위한 상태 변수
    std::vector<std::unique_ptr<T>> m_pool;     // 풀이 소유한 모든 객체
    std::vector<T*> m_available;                // 대여 가능한 객체

};

template<typename T>
ObjectPool<T>::ObjectPool(uint32_t size){
    preRelease = nullptr;
    addObject(size);
}

template<typename T>
typename ObjectPool<T>::ObjectPtr ObjectPool<T>::get(){

    std::unique_lock<std::mutex> lock(m_mutex);

    if(m_available.empty()){
        // 객체가 회수 될 때까지 대기
        cv.wait(lock , [this]{return !m_available.empty();});
    }

    T * obj = m_available.back();
    m_available.pop_back();

    return ObjectPtr(obj , [this](T * ptr){
        release(ptr);
    });
}


template<typename T>
void ObjectPool<T>::release(T * ptr){
    std::lock_guard<std::mutex> lock(m_mutex);
    m_available.push_back(ptr);
    cv.notify_one();
}

template<typename T>
void ObjectPool<T>::addObject(uint32_t size){
    for(int i = 0;i<size;i++){
        std::unique_ptr<T> newPtr = std::make_unique<T>();
        m_available.push_back(newPtr.get());
        m_pool.push_back(std::move(newPtr));
    }
}