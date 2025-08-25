# IOCP_lib
 ## 1. IOCP_lib 소개 (Introduction)
   
IOCP_lib는 C++17을 기반으로 하는 게임 서버 제작을 위한 IOCP(I/O Completion Port)
네트워크 라이브러리입니다.

이 라이브러리는 팀 프로젝트 'Space_Quick_Delevery'에서 처음 IOCP 서버를 구현하며 겪었던 한계점에서
출발했습니다. 당시에는 프로젝트에 종속적인 구조로 인해 코드의 재사용과 기능 확장이 어려웠습니다. 이러한
경험을 바탕으로, 복잡한 비동기 네트워크 로직과 객체 직렬화 등을 추상화하여 개발자가 핵심 콘텐츠 로직에만
집중할 수 있는 범용 라이브러리를 만들게 되었습니다.


✨ 주요 기능 (Features)

   - 비동기 I/O 및 논블로킹 네트워크
     - Windows의 IOCP API를 사용하여 수많은 클라이언트의 동시 접속을 최소한의 스레드로 효율적으로 처리합니다.

   - RPC (원격 프로시저 호출) 시스템
     - 간단한 함수 등록만으로 클라이언트의 요청에 응답하는 코드를 작성할 수 있어, 네트워크 이벤트 처리 로직이 단순해지고 유지보수성이 향상됩니다.

   - 리플렉션 기반 자동 직렬화/역직렬화
     - 기본 타입은 물론, 사용자 정의 클래스 객체까지 별도의 직렬화 코드 없이 네트워크를 통해 전송할 수 있습니다.

   - 객체 리플리케이션
     - 서버에서 관리되는 게임 오브젝트의 상태(생성, 소멸, 상태 변경)가 관련 클라이언트에게 자동으로
       동기화되어 데이터 일관성을 보장합니다.

   - 팩토리 패턴 기반 객체 관리
     - GameObject의 고유 ID만으로 해당 타입의 객체를 생성할 수 있어, 객체 타입에 의존하지 않는 유연한
       코드를 작성할 수 있습니다.


⚙️ 시작하기 (Getting Started)

  요구사항 (Prerequisites)

   - 운영체제: Windows
   - 컴파일러: C++17 표준을 지원하는 컴파일러 (e.g., Visual Studio 2017 이상, MSYS2/MinGW g++)
   - 링커 라이브러리: ws2_32, mswsock (Windows 소켓 API)

  설치 (Installation)

  본 라이브러리는 별도로 빌드하여 사용하는 방식이 아니며, 소스 파일과 헤더 파일을 프로젝트에 직접
  포함하여 사용합니다.

   1. 이 저장소의 cpp, header, utility 폴더를 사용자의 프로젝트 폴더로 복사합니다.
   2. 사용자의 빌드 시스템(e.g., Visual Studio, Makefile)에 다음을 설정합니다.
       - 소스 파일 포함: cpp 폴더와 utility/cpp 폴더에 있는 모든 .cpp 파일들을 컴파일 대상에 추가합니다.
       - 헤더 경로 포함: header 폴더와 utility/header 폴더를 헤더 검색 경로에 추가합니다.
       - 라이브러리 링크: 링커 설정에 ws2_32와 mswsock 라이브러리를 추가합니다.

🚀 사용법 및 예제 코드 (Usage & API Examples)

1. 서버 생성 및 실행

    가장 기본적인 서버를 생성하고 실행하는 코드입니다.
  ```cpp
  int main(){
    IocpServer iocp(9000); // 9000번 포트에서 실행

    if(iocp.Start()){
        std::cout<<"Server started on port 9000"<<std::endl;
    }else{
        std::cout<<"Failed to start server"<<std::endl;
    }
    while(1){
        std::string input;
        std::cin>>input;
        if(input == "exit"){
            iocp.Cleanup();
            break;
        }
    }
    return 0;
}
```

클라이언트에서 서버에 연결하는 코드입니다.

```cpp
int main(){
    
    IocpClient client;

    client.Start();


    if (!client.Connect(GAME,"192.168.0.102",9000)) { // 서버의 9000번 포트에 연결
        return 1;
    }
    while (true) {
        std::string msg;
        std::cout << "> ";
        std::cin>>msg;
        
        if (msg == "exit"){
            client.Cleanup();
            break;
        }
    }

    return 0;
}
```

2. RPC 함수 등록 및 호출
   클라이언트의 요청을 처리하는 RPC 함수를 등록하고, 클라이언트에서 이를 호출하는 예제입니다.
   
   공통 : 메세지 정의
   ```cpp
   // 반드시 BaseMessage 클래스를 상속
   class ChatMessage : public BaseMessage{
    public:
        uint32_t sessionId;
        std::string chat;

        // 메세지 id를 정의
        MESSAGE_IDENTIFICATION(21,ChatMessage)

        // 리플렉션 정보 추가
        REFLECTABLE(ChatMessage,
            MemberVariable("sessionId", Type::Int32, OffsetOf(ChatMessage, sessionId)),
    
            MemberVariable("chat", Type::String, OffsetOf(ChatMessage, chat))
        )
    };
   ```

   서버 사이드 : RPC 함수의 정의 및 등록
   ```cpp
   iocp.RpcRegist(new ChatMessage() , [&](BaseMessage * msg){
        // 기존 타입으로 캐스팅
        ChatMessage * cm = reinterpret_cast<ChatMessage*>(msg);

        // 연결된 모든 클라이언트에 발송
        auto connects = iocp.getConnect();
        for(auto it = connects.begin(); it != connects.end();it++){
            // 
            iocp.Send(it->first , cm);
        }
    });
   ```

   클라이언트 사이드 : RPC 호출
   ```cpp
   // 메세지 객체 생성
    std::shared_ptr<ChatMessage> cm = std::make_shared<ChatMessage>();

    // 정보 입력
    cm->sessionId = client.GetSessionId();
    cm->chat = msg;
    // 발송
    client.Send(GAME , cm);
   ```
3. 자동 직렬화를 위한 사용자 정의 클래스 정의
   
   메세지 내부에서 사용될 사용자 정의 클래스를 정의하는 방법입니다.

   BaseClass 혹은 GameObject를 상속해 사용 가능합니다.

    >BaseClass 상속의 경우 일반적으로 사용되는 클래스입니다. 반면 GameObject는 게임 내에서 사용될 게임 오브젝트의 정보를 다루기 위해 사용됩니다.
    ```cpp
    // BaseClass
    class Player : public BaseClass{
    public:
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
    ```
    ```cpp
    // GameObject
    class SampleObject : public GameObject{
    public:
        int32_t hp;
        int32_t mp;
        float x;
        float y;
        float z;

        OBJECT_IDENTIFICATION(202020 , SampleObject)

        REFLECTABLE(SampleObject,
            MemberVariable("hp" , Type::Int32 , OffsetOf(SampleObject , hp)),
            MemberVariable("mp" , Type::Int32 , OffsetOf(SampleObject , mp)),
            MemberVariable("x" , Type::Float , OffsetOf(SampleObject , x)),
            MemberVariable("y" , Type::Float , OffsetOf(SampleObject , y)),
            MemberVariable("z" , Type::Float , OffsetOf(SampleObject , z))
        )

    };
    ```