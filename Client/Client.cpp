#include <iostream>
#include <sstream>
#include <Winsock2.h>
#include <thread>
#include <future>
#include <string>
#include "OnlyThreadPool.h"

#pragma comment(lib, "ws2_32.lib")
using namespace std;

void recvData();


SOCKET sockClient;
SOCKADDR_IN addrClient;
ThreadPool pool;
stringstream convertType;
char port[5];
char ip[15];

int main() {

    int ret=-2;
    WSADATA wsaData;
    //初始化
    int nError = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (nError) {
        cout << "Load The Socket Word Stock Error..." << endl;
        return 0;
    }
    sockClient = socket(AF_INET, SOCK_STREAM, 0);
    addrClient.sin_family = AF_INET;
    while(ret!=0){
        cout << "请输入ip地址" << endl;
        cin.getline(ip, 15);
        addrClient.sin_addr.S_un.S_addr = inet_addr(ip);

        cout << "请输入端口号" << endl;
        cin.getline(port, 5);
        convertType << port;
        USHORT getPort;
        convertType >> getPort;
        addrClient.sin_port = htons(getPort);

        ret= connect(sockClient, (SOCKADDR *) &addrClient, sizeof(SOCKADDR));
        if (ret == -1) {
            cout<<"连接错误，请重新检查ip或端口号"<<endl;
        }
    }
    cout<<"连接成功"<<endl;
    pool.addTask(recvData);
    for (int index = 0;; index++) {
        if (SOCKET_ERROR == sockClient) {
            cout << "Build Socket Error:" << WSAGetLastError() << endl;
            return 0;
        }
        char sendBuf[255] = {"this is a"};
        cin.getline(sendBuf, 255);

//        string ch = "来自客户端:";
//        stringstream ss;
//        ss << addrClient.sin_port;
//        ch.append(ss.str()).append("--------->").append(sendBuf);
//
//        ch.copy(sendBuf, sizeof(sendBuf), 0);//将string的值copy给char数组
//        *(sendBuf + sizeof(sendBuf)) = '\0';//在末尾添加结束符

        send(sockClient, sendBuf, sizeof(sendBuf), 0);
        memset(sendBuf, '\0', sizeof(sendBuf));
        Sleep(2000);
    }

    WSACleanup();

    return 0;
}

void recvData() {
    char receive[255];
    while (true) {
        recv(sockClient, receive, 255, 0);
        if (strncmp("", receive, 1) == 0 || strncmp("\xcc", receive, 1) == 0 || strncmp("\0", receive, 1) == 0) {
            memset(receive, '\0', sizeof(receive));
            continue;
        }
        cout << receive << endl;
        memset(receive, '\0', sizeof(receive));
    }
}