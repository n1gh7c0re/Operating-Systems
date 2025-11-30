#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <cstring>
#include <string>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <ctime>
#include <iomanip>

// Константы для имен IPC
#define SHM_KEY_FILE "/tmp"
#define SHM_KEY_PROJ_ID 65
#define SEM_NAME_HOST "/sem_chat_host"
#define SEM_NAME_CLIENT "/sem_chat_client"
#define MQ_NAME_H2C "/mq_chat_h2c"
#define MQ_NAME_C2H "/mq_chat_c2h"
#define FIFO_NAME_H2C "/tmp/fifo_chat_h2c"
#define FIFO_NAME_C2H "/tmp/fifo_chat_c2h"

// Размер буфера
#define MSG_SIZE 256
#define TIMEOUT_SEC 5

enum MsgType {
    MSG_GENERAL = 1,
    MSG_PRIVATE = 2,
    MSG_DISCONNECT = 3 
};

struct ChatMessage {
    long mtype; 
    pid_t sender_pid;
    time_t timestamp;
    int type; // MsgType
    char content[MSG_SIZE];
};

// Логгер
inline void Log(const std::string& who, const std::string& msg) {
    time_t now = time(nullptr);
    auto tm = *localtime(&now);
    std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] " 
              << "[" << who << "] " << msg << std::endl;
}

// Проверка таймаута (для main loop)
inline bool IsTimeout(time_t last_act) {
    return (time(nullptr) - last_act) > 60;
}

#endif