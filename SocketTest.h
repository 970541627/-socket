//
// Created by 97054 on 2020/12/10.
//

#ifndef THREADPOOLBYONLY_SOCKETTEST_H
#define THREADPOOLBYONLY_SOCKETTEST_H

#include <locale>
#include "OnlyThreadPool.h"
#include<iostream>
#include <winsock2.h>
#include <vector>
#include <map>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

/**
 * 目前这个开了两个线程，一个主线程和一个子线程
 */
class Connect {
private:
    mutex mutex;
    WORD socketVersion;
    WSADATA wsaData;

    //创建套接字
    SOCKET sock;//服务端SOCKET

    using sockSetting = sockaddr_in;//给这个类起别名为socketSetting

    sockSetting sin;//

    char revData[255];//字符串类型的缓冲区，255大小
    vector<SOCKET> allConnection; //SOCKET容器，里面存放各种SOCKET对象
    map<SOCKET, sockSetting> ClientInformation;//通过SOCKET作为KEY，保存对应客户端的信息
    ThreadPool tPool; //线程池对象创建

    bool first = true;//判断是否是首次运行
    int nEventTotal = 0;

    WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];
    SOCKET sockArray[WSA_MAXIMUM_WAIT_EVENTS];
public:
    /**
     * 构造函数初始化，初始化连接WORAD，服务端SOCKET，以及将服务端SOCKET设置非阻塞模式
     * @param ul 参数1表示设置为非阻塞，0表示设置为阻塞,对应参数FIONBIO;
     */
    Connect() {


        WSAEVENT event = WSACreateEvent();
        WSAEventSelect(sock, event, FD_ACCEPT | FD_CLOSE);
        eventArray[nEventTotal] = event;
        sockArray[nEventTotal] = sock;
        ++nEventTotal;

        socketVersion = MAKEWORD(2, 2);

        if (WSAStartup(socketVersion, &wsaData) != 0) {
            return;
        }
        sock = socket(AF_INET, SOCK_STREAM, 0);

        unsigned long ul = 1;
        int ret = ioctlsocket(sock, FIONBIO, (unsigned long *) &ul);//设置成非阻塞模式。
        if (ret == SOCKET_ERROR)//设置失败。
        {
            cout << "fail";
        }
    }

    void setSOCK(ADDRESS_FAMILY sin_family,
                 USHORT sin_port,
                 char *add) {
        sin.sin_family = sin_family;
        sin.sin_port = htons(sin_port);
        sin.sin_addr.s_addr = inet_addr(add);

    }

    bool bindPort() {

        if (::bind(sock, (LPSOCKADDR) &sin, sizeof(sin)) == SOCKET_ERROR) {
            cout << "bind error" << endl;
            return false;
        }
        //开始监听
        if (listen(sock, 5) == SOCKET_ERROR) {
            printf("listen error !");
            return false;
        }
        cout << "服务器已经启动" << endl;
        return true;
        //tPool.addTask(bind(&Connect::go, this), NULL);

    }

    /**
     * 接收客户端连接
     */
    [[noreturn]] void connectS() {
        while (true) {
            //int nIndex = WSAWaitForMultipleEvents(nEventTotal, eventArray, FALSE, WSA_INFINITE, FALSE);
            SOCKET acceptData;
            sockSetting remoteAddr;

            int Addrlen = sizeof(remoteAddr);

            acceptData = ::accept(sock, (sockaddr *) &remoteAddr, &Addrlen);

            if (acceptData == INVALID_SOCKET && acceptData != NULL) {
                Sleep(1000);
                //cout << "accept error" << endl;
                continue;
            }

            allConnection.emplace_back(acceptData);
            ClientInformation.emplace(acceptData, remoteAddr);
            cout << "来自客户端" << inet_ntoa(remoteAddr.sin_addr) << "------->" << ntohs(remoteAddr.sin_port) << "开始了连接"
                 << endl;

            if (first) {
                tPool.addTask(bind(&Connect::myRecv, this), NULL);
                first = false;
            }
            Sleep(3000);
        }
    }

    void myRecv() {
        fd_set fdRead;

        while (true) {
            for (int i = 0; i < allConnection.size(); ++i) {
                FD_ZERO(&fdRead);
                FD_SET(allConnection[i], &fdRead);
                timeval t = {1, 0};
                int ret = ::select(sock + 1, &fdRead, NULL, NULL, &t);
                switch (ret) {
                    case 0:
                        continue;
                    case -1:
                        allConnection.erase(allConnection.begin() + i);
                        cout << "客户端断开" << endl;
                        break;

                    default:
                        if (FD_ISSET(allConnection.at(i), &fdRead)) {
                            if (dataHandler(i) != -1) {
                                for (int i = 0; i < allConnection.size(); i++) {
                                    send(allConnection.at(i), revData, 255, 0);
                                }
                                memset(revData, '\0', sizeof(revData));
                            } else {
                                allConnection.erase(allConnection.begin() + i);
                                cout << "客户端连接断开\n";
                                continue;
                            }
                        }
                        break;
                }
            }
        }
    };

    /**
     * 接收来自客户端发送的数据,
     * 目前客户端只单方面写了发送，并没有写接收信息。
     * 方法Sleep：休眠当前线程500毫秒
     * 方法sendMessage：用来检测客户端是否断开连接
     * （通过给客户端发送数据包来检测，send函数若返回-1就表示失败，表示0就表示发送成功，以此来判断客户端是否断开连接)
     */
    int dataHandler(int &i) {
        int ret = recv(allConnection.at(i), revData, sizeof(revData), 0);
        if (strncmp("", revData, 1) == 0 || strncmp("\0", revData, 1) == 0 || strncmp("\xcc", revData, 1) == 0) {
            memset(revData, '\0', sizeof(revData));
            return ret;
        }

        //取出当前socket的用户信息
        sockSetting get = ClientInformation.at(allConnection.at(i));
        cout << "来自客户端" << inet_ntoa(get.sin_addr) << "------->" << ntohs(get.sin_port) << "的消息 : " << revData
             << endl;

        Sleep(500);
        return ret;
    }
};


#endif //THREADPOOLBYONLY_SOCKETTEST_H
