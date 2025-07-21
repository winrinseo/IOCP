
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "outputMemoryStream.h"
#include "inputMemoryStream.h"
#include "iocpClient.h"


using namespace std;

// #pragma comment(lib, "ws2_32.lib")



void chatThread(){
    std::cout<<"chatThread 시작"<<"\n";
    while(1){
        uint32_t id = GameObjectManager::Get()->PopUpdatedNetworkId();
        Chat * chat = (Chat *)GameObjectManager::Get()->ObjectToAddress(id);
        std::cout<<chat->sessionId<<" : "<<chat->chat<<"\n";
    }
}

int main(){
    SetConsoleOutputCP(CP_UTF8);  // 콘솔 출력 인코딩을 UTF-8로 설정
    SetConsoleCP(CP_UTF8); 
    
    IocpClient client;

    OutputMemoryStream outputStream;
    std::shared_ptr<Command> cmd = std::make_shared<Command>();
    cmd->num = 4;
    Player m; m.mId = 168; m.mName = "winrinseo"; m.mScore = {6,10,11};
    Player p; p.mId = 1; p.mName = "우린"; p.mScore = {4,5};
    Player p1; p1.mId = 2; p1.mName = "dnfls"; p1.mScore = {2,7};
    cmd->me = &m;
    cmd->cmdDeck.push_back(&p);
    cmd->cmdDeck.push_back(&p1);

    
    client.SetReceiveProcess([&](uint32_t & sessionId , const char * buffer , DWORD bytesTransferred){
        // cout<<"에코 완료 ! "<<string(buffer , bytesTransferred)<<"\n";
        return;
    });

    client.SetSendProcess([&](){
        cout<<"전송 완료 !!\n";
        return;
    });

    client.Start();


    if (!client.Connect(GAME,"192.168.0.102",9000)) {
        return 1;
    }

    ReplicationData * r = new ReplicationData();
    SampleObject sp;
    SampleObject2 sp2;
    GameObjectWrapper o;
    GameObjectWrapper o2;

    o.networkId = 2;
    o.objectId = sp.GetObjectId();
    o.obj = &sp;
    sp.hp = 200;
    sp.mp = 140;
    sp.x = 3.14;
    sp.y = 2.4;
    sp.z = 128.3;

    o2.networkId = 3;
    o2.objectId = sp2.GetObjectId();
    o2.obj = &sp2;

    sp2.x = 3.1;
    sp2.y = 2.44;
    sp2.z = 128.311;

    r->sessionId = client.GetSessionId();
    r->networkGroup = 110011;
    r->objList.push_back(&o);
    r->objList.push_back(&o2);

    int room = 0;
    std::cin>>room;

    IntoNetworkGroup * into = new IntoNetworkGroup();

    into->sessionId = client.GetSessionId();
    into->networkGroup = room;

    client.Send(GAME , into);

    std::vector<std::thread> th;
    th.emplace_back([]{chatThread();});
    
    std::cout<<"나의 세션 아이디 : "<<into->sessionId<<"\n";
    std::cout<<"나의 네트워크 그룹 : "<<room<<"\n";
    while (true) {
        std::string msg;
        std::cout << "> ";
        std::cin>>msg;
        
        if (msg == "exit")
        break;
        
        std::shared_ptr<ChatMessage> ch = std::make_shared<ChatMessage>();

        ch->sessionId = client.GetSessionId();
        ch->networkGroup = room;
        ch->chat = msg;

        client.Send(GAME , ch);

    }

    return 0;
}