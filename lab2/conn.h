#ifndef CONN_H
#define CONN_H

#include "common.h"

class Conn {
public:
    // is_host: true если процесс Хост, false если Клиент
    Conn(pid_t remote_pid, bool is_host);
    virtual ~Conn();

    // Возвращает true, если успешно записал. false при ошибке.
    bool Write(const ChatMessage& msg);

    // Возвращает true, если сообщение прочитано. 
    // Возвращает false, если пусто или таймаут.
    bool Read(ChatMessage& msg);

private:
    struct Impl;
    Impl* pImpl;
};

#endif