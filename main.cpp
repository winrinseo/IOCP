
#include <iostream>
#include "iocpServer.h"

// #pragma comment(lib, "ws2_32.lib")

int main(){

    SetConsoleOutputCP(CP_UTF8);  // 콘솔 출력 인코딩을 UTF-8로 설정
    SetConsoleCP(CP_UTF8); 

    IocpServer iocp(9000);
    iocp.Start();
    while(1){}
    return 0;
}