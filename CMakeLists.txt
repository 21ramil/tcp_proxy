# Минимальная версия
cmake_minimum_required(VERSION 3.22)

# Наименование проекта, язык программирования, версия проекта
project(TCPProxy VERSION 1.0 LANGUAGES CXX)

# Используемый стандарт С++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Создание исполняемого файла из исходников
add_executable(tcp_proxy 
							main.cpp 
							
							proxyserver.cpp
							proxyserver.h
				)

# Добавление дополнительного каталога поиска загловочных файлов. Необходим для файла с конфигурационными данными
target_include_directories(tcp_proxy PRIVATE
							${PROJECT_BINARY_DIR}
                           )
