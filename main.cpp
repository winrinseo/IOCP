
#include <iostream>
#include "iocpServer.h"

// #pragma comment(lib, "ws2_32.lib")

void f(BaseMessage * msg){
    Command * cmd = (Command*) msg;
    std::cout<<"내 정보 : "<<cmd->me->mId<<" "<<cmd->me->mName<<" ";
    for(int j = 0;j<cmd->me->mScore.size(); j++)
            std::cout<<cmd->me->mScore[j]<<" ";
    std::cout<<"\n";
    for(int i = 0;i<cmd->cmdDeck.size();i++){
            std::cout<<"팀 정보 : "<<cmd->cmdDeck[i]->mId<<" "<<cmd->cmdDeck[i]->mName<<" ";
            for(int j = 0;j<cmd->cmdDeck[i]->mScore.size(); j++)
                std::cout<<cmd->cmdDeck[i]->mScore[j]<<" ";
            std::cout<<"\n";
    }

    return;
}

int main(){

    SetConsoleOutputCP(CP_UTF8);  // 콘솔 출력 인코딩을 UTF-8로 설정
    SetConsoleCP(CP_UTF8); 

    IocpServer iocp(9000);


    iocp.RpcRegist(new Command() , f);

    iocp.RpcRegist(new ChatMessage() , [&](BaseMessage * msg){
        ChatMessage * chat = (ChatMessage*) msg;

        Chat * chatObject = new Chat();
        chatObject->sessionId = chat->sessionId;
        chatObject->chat = chat->chat;

        std::cout<<chatObject->chat<<"\n";
        GameObjectManager::Get()->CreateObject(chat->networkGroup , chatObject);

    });




    iocp.SetReceiveProcess([&](uint32_t & sessionId ,const char * buffer , DWORD bytesTransferred){
        // std::cout<<"RECEIVE!!"<<" ";
        
        // InputMemoryStream inputStream;
        // inputStream.Prepare((char*)buffer , bytesTransferred);
        // //1바이트 역직렬화 하고
        // uint8_t mId;
        // inputStream.Serialize(&mId , sizeof(uint8_t));
        // std::cout<<inputStream.mHead<<"\n";

        // //메세지 관리자에서 번호보고 객체 생성
        // Command * cmd = new Command();

        // //역직렬화
        // inputStream.SerializeMessage((BaseMessage *)cmd);

        // //원격 프로시저 호출
        // for(int i = 0;i<cmd->cmdDeck.size();i++){
        //     std::cout<<"정보 : "<<cmd->cmdDeck[i]->mId<<" "<<cmd->cmdDeck[i]->mName<<" "<<
        //     cmd->cmdDeck[i]->mScore[0]<<" "<<cmd->cmdDeck[i]->mScore[1]<<" ";
            
        // }
        // std::cout<<"\n";
        // session->Send(buffer , bytesTransferred);
        // // std::cout<<"수신 됨 : "<<std::string(buffer , bytesTransferred)<<"\n";
        return;
    });

    iocp.Start();
    while(1){}
    return 0;
}