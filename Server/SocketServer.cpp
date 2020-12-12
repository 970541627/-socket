//
// Created by 97054 on 2020/12/11.
//


#include "SocketServer.h"
#include <string>

using namespace std;

int main(int argc, char *argv[]) {

    Connect con;
    char ip[15];
    char port[5];
    int size = sizeof(port) / sizeof(port[0]);
    string ch;
    USHORT p;
    bool isSuccess = false;

    while (!isSuccess) {
        ch = "";
        cout << "请输入需要连接的ip地址" << endl;
        cin.getline(ip, 15);
        cout << "请输入任意开放的端口号" << endl;
        cin.getline(port, 5);
        for (int i = 0; i < size; i++) {
            ch += port[i];
        }
        p = atoi((char *) ch.c_str());
        con.setSOCK(AF_INET, p, ip);
        isSuccess = con.bindPort();
    }
    con.connectS();

    return 0;


}

