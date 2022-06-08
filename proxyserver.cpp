#include "proxyserver.h"

#include <stdio.h>
#include <stdlib.h>   
#include <fcntl.h>

#include <algorithm>
#include <set>
#include <string>
#include <map>
#include <ctime>
#include <iostream>

bool ProxyServer::set_nonblock(int fd)
{
    DWORD nonBlocking = 1;
    if (ioctlsocket(fd, FIONBIO, &nonBlocking) != 0)
    {
        fout << getDateTime() <<"failed to set non-blocking socket."<<std::endl;
        return false;
    }

    return true;
}

ProxyServer::ProxyServer(int proxyPort, const char* connectIPAddress, int serverPort, std::string logFileName)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)         
        fout << getDateTime()<<"WSAStartup failed with error: "<< err<<std::endl;
    
    // создаем основной сокет для прокси, который будет работать с ipv4
    // и иметь тип подключения TCP
    MainSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (MainSocket == INVALID_SOCKET)
        fout << getDateTime() << "Error " << WSAGetLastError() << std::endl;
   
    setServerConfig(connectIPAddress, serverPort);
    setSocketConfig(proxyPort);
    // открываем файл для записи логов
    if (!logFileName.empty())
        fout.open(logFileName, std::ofstream::out | std::ofstream::app);
    if (!fout)
    {
        fout << getDateTime() << "Error in opening file for writing."<<std::endl;
        exit(-1);
    }
}
ProxyServer::~ProxyServer()
{
    fout << getDateTime() << ": Proxy killed." << std::endl;
    fout.flush();
    fout.close();
    closesocket(MainSocket);
    closesocket(ServerSocket);
}
void ProxyServer::setServerConfig(const char *connectIPAddress, int port)
{
    _sockServerAddr.sockAddr.sin_family = AF_INET;   // домен
    _sockServerAddr.sockAddr.sin_port = htons(port);  // порт
    _sockServerAddr.sockAddr.sin_addr.s_addr = inet_addr(connectIPAddress);
}

void ProxyServer::setSocketConfig(int port)
{
    _sockMasterAddr.sockAddr.sin_family = AF_INET;
    _sockMasterAddr.sockAddr.sin_port = htons(port);
    _sockMasterAddr.sockAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
       
}

std::string ProxyServer::getDateTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return std::string(buf);   
}

int ProxyServer::connectToServer(int &connectServer)
{
    // дескриптор = socket(домен, тип, протокол)
    ServerSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (ServerSocket == INVALID_SOCKET)
    {
        int error = WSAGetLastError();
        fout << getDateTime() << ": Error connect to server." << error << std::endl;
        return -1;
    }
    // соединяем прокси-клиента с сервером
    connectServer = connect(ServerSocket, (struct sockaddr*)(&_sockServerAddr), sizeof(_sockServerAddr));

    set_nonblock(ServerSocket);
    if (connectServer == 0)
        fout << getDateTime() << ": Proxy connect to server." << std::endl;
    else
    {
        fout << getDateTime() << ": Error connect to server." << std::endl;
        return -1;
    }
    return 0;
}
int ProxyServer::run()
{
    fout << getDateTime() << ": Proxy started." << std::endl;
    // привязываем сокет к порту и ip адресу
    bind(MainSocket, (struct sockaddr*)(&_sockMasterAddr), sizeof(_sockMasterAddr));

    // множество прослушиваемых сокетов
    std::set<int> slaveSockets;
    // мэп с данными прослушиваемых сокетов
    std::map<int, SockAddr> socketsClient;

    set_nonblock(MainSocket);
    listen(MainSocket, SOMAXCONN);

    // связываем прокси с сервером
    int connectServer = 0;
    if (connectToServer(connectServer) <0)
        return -1;

    while (true)
    {
        Buffer.clear();
        // создаем набор дескрипторов
        fd_set set;
        FD_ZERO(&set); 
        FD_SET(MainSocket, &set);

        for (auto iter = slaveSockets.begin(); iter != slaveSockets.end(); ++iter)
        {
            FD_SET(*iter, &set);
        }
            
        int Max = MainSocket;
        if (slaveSockets.size() > 0)
        {
            Max = max(MainSocket, *std::max_element(slaveSockets.cbegin(), slaveSockets.cend()));
        }
        select(Max + 1, &set, &set, NULL, NULL);
           
        for (auto iter = slaveSockets.begin(); iter != slaveSockets.end(); )
        {
            if (FD_ISSET(*iter, &set))
            {
                char* tmpBuffer = new char[bufferSize];
                int recvSize = recv(*iter, tmpBuffer, bufferSize, 0);

                if ((recvSize == 0) && (errno != EAGAIN))
                {
                    std::string msg = "Client ";
                    char* ip = inet_ntoa(socketsClient[*iter].sockAddr.sin_addr);
                    msg += ip;
                    msg += " deleted.";
                    fout <<getDateTime()<<": "<< msg << std::endl;

                    int it = *iter;
                    shutdown(*iter, SD_BOTH);
                    closesocket(*iter);

                    socketsClient.erase(it);
                    iter = slaveSockets.erase(iter);
                }
                else if (recvSize > 0)
                {
                    Buffer = std::string(tmpBuffer, recvSize);

                    std::string msg = "From client ";
                    char* ip = inet_ntoa(socketsClient[*iter].sockAddr.sin_addr);
                    msg += ip;
                    send(ServerSocket, Buffer.c_str(), Buffer.length(), 0);
                    fout << getDateTime()<<": " << msg << ": " << Buffer << std::endl;
                    ++iter;
                }
                else
                    ++iter;
                delete[]tmpBuffer;

            }
            else
                ++iter;
        }

        if (FD_ISSET(MainSocket, &set))
        {
            struct sockaddr_in sockAddr;
            socklen_t saddrlen = sizeof(sockAddr);
            int slaveSocket = accept(MainSocket, (sockaddr*)&sockAddr, &saddrlen);
            set_nonblock(slaveSocket);
            slaveSockets.insert(slaveSocket);
            socketsClient[slaveSocket] = SockAddr{ sockAddr, saddrlen };
            std::string msg = "Add new client ";
            char* ip = inet_ntoa(socketsClient[slaveSocket].sockAddr.sin_addr);
            msg += ip;
            fout << getDateTime() << ": " << msg << std::endl;        
        }
                
        // если установлена связь прокси с сервером
        if (connectServer == 0)
        {
            char *tmpBuffer = new char[bufferSize];
            // получаем сообщение от сервера
            int recvSizeServer = recv(ServerSocket, tmpBuffer, bufferSize, 0);
                
            if ((recvSizeServer == 0) && (errno != EAGAIN))
            {
                std::string msg = "Server ";
                char* ip = inet_ntoa(socketsClient[ServerSocket].sockAddr.sin_addr);
                msg += ip;
                msg += " deleted.";
                fout << getDateTime()<<": " << msg << std::endl;
                for (auto iter = slaveSockets.begin(); iter != slaveSockets.end(); ++iter)
                {
                    send(*iter, msg.c_str(), msg.length(), 0);
                }
                 
                socketsClient.erase(ServerSocket);
                shutdown(ServerSocket, SD_BOTH);
                closesocket(ServerSocket);
                    
            }
            else if (recvSizeServer > 0)
            {
                Buffer = std::string(tmpBuffer, recvSizeServer);
                    
                for (auto iter = slaveSockets.begin(); iter != slaveSockets.end(); ++iter)
                {
                    // отправка сообщения от сервера клиентам
                    send(*iter, Buffer.c_str(), Buffer.length(), 0);

                    std::string msg = "From server to client  ";
                    char* ip = inet_ntoa(socketsClient[*iter].sockAddr.sin_addr);
                    msg += ip;
                    fout << getDateTime()<<": " << msg << ": " << Buffer << std::endl;
                }
            }
            delete[]tmpBuffer;
                
        }
    }
    closesocket(MainSocket);
    WSACleanup();
}

