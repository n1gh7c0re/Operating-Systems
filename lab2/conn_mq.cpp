#include "conn.h"
#include <mqueue.h>
#include <errno.h>

struct Conn::Impl {
    mqd_t mq_read;
    mqd_t mq_write;
    bool is_host;

    Impl(bool host) : is_host(host) {
        struct mq_attr attr;
        attr.mq_flags = 0;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = sizeof(ChatMessage);
        attr.mq_curmsgs = 0;

        int flags = O_RDWR | O_CREAT;
        if (!is_host) flags = O_RDWR; 

        if (is_host) {
            mq_unlink(MQ_NAME_H2C);
            mq_unlink(MQ_NAME_C2H);
            
            mq_read = mq_open(MQ_NAME_C2H, flags, 0666, &attr); 
            mq_write = mq_open(MQ_NAME_H2C, flags, 0666, &attr); 
        } else {
            mq_read = mq_open(MQ_NAME_H2C, flags); 
            mq_write = mq_open(MQ_NAME_C2H, flags); 
        }

        if (mq_read == (mqd_t)-1 || mq_write == (mqd_t)-1) {
            perror("mq_open");
            exit(1);
        }
    }

    ~Impl() {
        mq_close(mq_read);
        mq_close(mq_write);
        if (is_host) {
            mq_unlink(MQ_NAME_H2C);
            mq_unlink(MQ_NAME_C2H);
        }
    }
};

Conn::Conn(pid_t pid, bool host) : pImpl(new Impl(host)) {}
Conn::~Conn() { delete pImpl; }

bool Conn::Write(const ChatMessage& msg) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += TIMEOUT_SEC;

    if (mq_timedsend(pImpl->mq_write, (const char*)&msg, sizeof(msg), 0, &ts) == -1) {
        return false;
    }
    return true;
}

bool Conn::Read(ChatMessage& msg) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;

    ssize_t bytes = mq_timedreceive(pImpl->mq_read, (char*)&msg, sizeof(msg), nullptr, &ts);
    return bytes > 0;
}