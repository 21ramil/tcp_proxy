#ifndef UNICODE
    #define UNICODE 1
#endif
#if WIN32
// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <fstream>

class ProxyServer
{
public:
    /*
     * @brief конструктор
     * @param proxyPort - порт, который прослушивает прокси сервер, 
     * @param const char* connectIPAddress - IP сервера, куда подключается прокси сервер,
     * @param int serverPort - порт сервера, куда подключается прокси сервер,
     * @param std::string logFileName - наименование файла, куда прокотолирует прокси сервер
    */ 
    ProxyServer(int proxyPort, const char* connectIPAddress, int serverPort, std::string logFileName);
    ~ProxyServer();

    /*
     * @brief запуск прокси сервера
    */
    int run();

private:
    //Структура описывает сокет для работы с протоколами IP
    struct SockAddr
    {
        sockaddr_in sockAddr;
        int adrrelen;
    };
    //Структура описывает сокет для работы с протоколами IP для прокси
    SockAddr _sockMasterAddr;
    //Структура описывает сокет для работы с протоколами IP для сервера
    SockAddr _sockServerAddr;
    // номер дескриптора прокси
    int MainSocket;
    // номер дескриптора сервера
    int ServerSocket;
    const int bufferSize = 1024;
    std::fstream fout;
    // буфер для получаемых/отправляемых сообщений
    std::string Buffer;
    
    /*
     * @brief установка сокета неблокирующим
     * @param fd дескриптор сокета    
     * @return true - установка прошла успешно, иначе false
    */
    bool set_nonblock(int fd);

    /*
     * @brief установить конфигурации сервера
     * @param connectIPAddress - ip адрес
     * @param port - номер порта
    */
    void setServerConfig(const char* connectIPAddress, int port);

    /*
     * @brief установить конфигурации прокси
     * @param port - номер порта
    */
    void setSocketConfig(int port);
   
    /*
     * @brief получить текущее дата и время
     * @return время в формате ГГГГ-ММ-ДД ЧЧ:ММ:СС
    */
    std::string getDateTime();

    /*
     * @brief подключиться к серверу
     * @param connectServer - флаг установки соединения прокси с сервером
     * @return 0 - соединение с сервером установлено, иначе -1
    */ 
    int connectToServer(int& connectServer);

};
