#include "proxyserver.h"

#include <iostream>
#include <string>

#if __has_include("windows.h")
    #include <windows.h>
#endif

/**
 * @brief Вывод краткой справки об использовании приложения
 *
 * @param applicationName Наименование приложения
 *
 */
void help(const std::string_view applicationName)
{
    std::cout << "Запуск проксисервера со входными аргументами:\n";
    std::cout << "Использовать:" << applicationName << " -ip [connectIPAddress] -p [connectPort] -l [listenPort] -o [logfile]\n";
    std::cout << "-ip, --ip_address [connectIPAddress]\t- IP сервера, куда подключается прокси сервер\n";
    std::cout << "-p,  --connect_port [connectPort]\t- порт сервера, куда подключается прокси сервер\n";
    std::cout << "-l,  --listen_port [listenPort]\t\t- порт, который прослушивает прокси сервер\n";
    std::cout << "-o,  --output_file [logfile]\t\t- наименование файла, куда прокотолирует прокси сервер\n";
}


/**
 * @brief Точка входа в программу
 *
 * После запуска программы запускается данная функция.
 *
 * @param argc Количество аргументов входной строки
 * @param argv Сами аргументы
 *
 * @return Код выхода из программы
 */
int main(int argc, char* argv[])
{

#ifdef WIN32
    SetConsoleOutputCP(65001);
#endif
    
    std::string applicationName;
 
    // получаем наименование программы
    if (argc >= 1)
    {
        applicationName = argv[0];
        #if WIN32
            applicationName = applicationName.substr(applicationName.find_last_of('\\') + 1);
        #else
            applicationName = applicationName.substr(applicationName.find_last_of('/'));
        #endif
    }

    // если количество входных параметров нас не устраивает выводим справку и завершаем приложение с ошибкой
    if (argc != 9) 
    {
        std::cout << "В командной строке необходимо ввести корректные параметры для прокси сервера. Попробуйте снова.\n";
        help(applicationName);

        exit(1);
    }

    // обработка входных параметров
    const char* connectIPAddress;
    const char* connectPort;
    const char* listenPort;
    const char* logfile;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "-ip") || (arg == "--ip_address"))
        {
            connectIPAddress = argv[++i];
        }
        else if ((arg == "-p") || (arg == "--connect_port"))
        {
            connectPort = argv[++i];
        }
        else if ((arg == "-l") || (arg == "--listen_port"))
        {
            listenPort = argv[++i];
        }
        else if ((arg == "-o") || (arg == "--output_file"))
        {
            logfile = argv[++i];
        }
    }

 
    // создание экземпляра класса прокси сервера
    ProxyServer proxy(std::stoi(listenPort), connectIPAddress, std::stoi(connectPort), logfile);
    
    // запуск прокси сервера
    proxy.run();

    return 0;
}
