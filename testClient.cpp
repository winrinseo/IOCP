
#include <iostream>
#include <string>
#include "outputMemoryStream.h"
#include "inputMemoryStream.h"
#include "iocpClient.h"


using namespace std;

// #pragma comment(lib, "ws2_32.lib")

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
        cout<<"전송 완료 !!";
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

    int i = 0;
    
    while (true) {
        std::string msg;
        std::cout << "> ";
        std::getline(std::cin, msg);
        
        if (msg == "exit")
        break;
        
        if(i % 2 ==0) client.Send(GAME , r);

        if(i % 2 ==1) client.Send(GAME , cmd);
        i++;

        // if (!client.SendToServer(msg)) {
        //     break;
        // }

        // if (!client.ReceiveMessage()) {
        //     break;
        // }
    }

    return 0;
}