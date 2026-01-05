cmake_minimum_required(VERSION 3.10)
project(LinuxIPC_Chat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread -lrt")

# Список реализаций
file(GLOB CONN_SOURCES "conn_*.cpp")

foreach(CONN_SRC ${CONN_SOURCES})
    get_filename_component(FILENAME ${CONN_SRC} NAME_WE)
    string(REPLACE "conn_" "" TYPE_CODE ${FILENAME})

    add_executable(host_${TYPE_CODE} host.cpp ${CONN_SRC})
    
    add_executable(client_${TYPE_CODE} client.cpp ${CONN_SRC})
endforeach()