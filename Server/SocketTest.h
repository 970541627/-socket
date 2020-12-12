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
 * Ŀǰ������������̣߳�һ�����̺߳�һ�����߳�
 */
class Connect {
private:
    mutex mutex;
    WORD socketVersion;
    WSADATA wsaData;

    //�����׽���
    SOCKET sock;//�����SOCKET

    using sockSetting = sockaddr_in;//������������ΪsocketSetting

    sockSetting sin;//

    char revData[255];//�ַ������͵Ļ�������255��С
    vector<SOCKET> allConnection; //SOCKET�����������Ÿ���SOCKET����
    map<SOCKET, sockSetting> ClientInformation;//ͨ��SOCKET��ΪKEY�������Ӧ�ͻ��˵���Ϣ
    ThreadPool tPool; //�̳߳ض��󴴽�

    bool first = true;//�ж��Ƿ����״�����
    int nEventTotal = 0;

    WSAEVENT eventArray[WSA_MAXIMUM_WAIT_EVENTS];
    SOCKET sockArray[WSA_MAXIMUM_WAIT_EVENTS];
public:
    /**
     * ���캯����ʼ������ʼ������WORAD�������SOCKET���Լ��������SOCKET���÷�����ģʽ
     * @param ul ����1��ʾ����Ϊ��������0��ʾ����Ϊ����,��Ӧ����FIONBIO;
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
        int ret = ioctlsocket(sock, FIONBIO, (unsigned long *) &ul);//���óɷ�����ģʽ��
        if (ret == SOCKET_ERROR)//����ʧ�ܡ�
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
        //��ʼ����
        if (listen(sock, 5) == SOCKET_ERROR) {
            printf("listen error !");
            return false;
        }
        cout << "�������Ѿ�����" << endl;
        return true;
        //tPool.addTask(bind(&Connect::go, this), NULL);

    }

    /**
     * ���տͻ�������
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
            cout << "���Կͻ���" << inet_ntoa(remoteAddr.sin_addr) << "------->" << ntohs(remoteAddr.sin_port) << "��ʼ������"
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
                        cout << "�ͻ��˶Ͽ�" << endl;
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
                                cout << "�ͻ������ӶϿ�\n";
                                continue;
                            }
                        }
                        break;
                }
            }
        }
    };

    /**
     * �������Կͻ��˷��͵�����,
     * Ŀǰ�ͻ���ֻ������д�˷��ͣ���û��д������Ϣ��
     * ����Sleep�����ߵ�ǰ�߳�500����
     * ����sendMessage���������ͻ����Ƿ�Ͽ�����
     * ��ͨ�����ͻ��˷������ݰ�����⣬send����������-1�ͱ�ʾʧ�ܣ���ʾ0�ͱ�ʾ���ͳɹ����Դ����жϿͻ����Ƿ�Ͽ�����)
     */
    int dataHandler(int &i) {
        int ret = recv(allConnection.at(i), revData, sizeof(revData), 0);
        if (strncmp("", revData, 1) == 0 || strncmp("\0", revData, 1) == 0 || strncmp("\xcc", revData, 1) == 0) {
            memset(revData, '\0', sizeof(revData));
            return ret;
        }

        //ȡ����ǰsocket���û���Ϣ
        sockSetting get = ClientInformation.at(allConnection.at(i));
        cout << "���Կͻ���" << inet_ntoa(get.sin_addr) << "------->" << ntohs(get.sin_port) << "����Ϣ : " << revData
             << endl;

        Sleep(500);
        return ret;
    }
};


#endif //THREADPOOLBYONLY_SOCKETTEST_H
