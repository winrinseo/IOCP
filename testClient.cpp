
#include <iostream>
#include "asyncClient.h"

// #pragma comment(lib, "ws2_32.lib")

int main(){
    
    AsyncClient client("127.0.0.1",9000);


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

        if (!client.ReceiveMessage()) {
            break;
        }
    }

    return 0;
}