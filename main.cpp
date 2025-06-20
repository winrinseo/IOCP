
#include <iostream>
#include "iocpServer.h"

// #pragma comment(lib, "ws2_32.lib")

int main(){

    SetConsoleOutputCP(CP_UTF8);  // 콘솔 출력 인코딩을 UTF-8로 설정
    SetConsoleCP(CP_UTF8); 

    IocpServer iocp(9000);
    iocp.SetReceiveProcess([&](Session * session ,const char * buffer , DWORD bytesTransferred){

        session->Send(buffer , bytesTransferred);
        std::cout<<"수신 됨 : "<<std::string(buffer , bytesTransferred)<<"\n";
        return;
    });

    iocp.Start();
    while(1){}
    return 0;
}