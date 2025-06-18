
#include <iostream>
#include "asyncClient.h"



// #pragma comment(lib, "ws2_32.lib")

int main(){
    SetConsoleOutputCP(CP_UTF8);  // 콘솔 출력 인코딩을 UTF-8로 설정
    SetConsoleCP(CP_UTF8); 
    
    
    AsyncClient client("192.168.0.102",9000);


    if (!client.Connect()) {
        return 1;
    }

    while (true) {
        std::string msg;
        std::cout << "> ";
        std::getline(std::cin, msg);

        if (msg == "exit")
            break;

        if (!client.SendToServer(msg)) {
            break;
        }

        // if (!client.ReceiveMessage()) {
        //     break;
        // }
    }

    return 0;
}