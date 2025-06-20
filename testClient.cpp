
#include <iostream>
#include <string>
#include "iocpClient.h"

using namespace std;

// #pragma comment(lib, "ws2_32.lib")

int main(){
    SetConsoleOutputCP(CP_UTF8);  // 콘솔 출력 인코딩을 UTF-8로 설정
    SetConsoleCP(CP_UTF8); 
    
    
    IocpClient client;
    
    client.SetReceiveProcess([&](Session * session , const char * buffer , DWORD bytesTransferred){
        cout<<"에코 완료 ! "<<string(buffer , bytesTransferred)<<"\n";
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

    for( auto session : client.servers_){
        cout<<session<<"\n";
    }

    while (true) {
        std::string msg;
        std::cout << "> ";
        std::getline(std::cin, msg);

        if (msg == "exit")
            break;

        client.getServer(GAME)->Send(msg.c_str() , msg.size());

        // if (!client.SendToServer(msg)) {
        //     break;
        // }

        // if (!client.ReceiveMessage()) {
        //     break;
        // }
    }

    return 0;
}