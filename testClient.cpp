
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
    Player::InitDataType();
    Command::InitDataType();
    
    IocpClient client;

    OutputMemoryStream outputStream;
    Command * cmd = new Command();
    cmd->num = 4;
    Player m; m.mId = 168; m.mName = "winrinseo"; m.mScore = {6,10,11};
    Player p; p.mId = 1; p.mName = "우린"; p.mScore = {4,5};
    Player p1; p1.mId = 2; p1.mName = "dnfls"; p1.mScore = {2,7};
    cmd->me = &m;
    cmd->cmdDeck.push_back(&p);
    cmd->cmdDeck.push_back(&p1);

    outputStream.Prepare();
    outputStream.SerializeMessage((BaseMessage*)cmd);
    
    client.SetReceiveProcess([&](Session * session , const char * buffer , DWORD bytesTransferred){
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

    while (true) {
        std::string msg;
        std::cout << "> ";
        std::getline(std::cin, msg);

        if (msg == "exit")
            break;

        client.getServer(GAME)->Send(outputStream.GetBuffer() , outputStream.GetLength());

        // if (!client.SendToServer(msg)) {
        //     break;
        // }

        // if (!client.ReceiveMessage()) {
        //     break;
        // }
    }

    return 0;
}