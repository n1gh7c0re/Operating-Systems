#include "conn.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

struct Conn::Impl {
    int fd_read;
    int fd_write;
    bool is_host;

    Impl(bool host) : is_host(host) {
        if (is_host) {
            mkfifo(FIFO_NAME_H2C, 0666);
            mkfifo(FIFO_NAME_C2H, 0666);
            fd_write = open(FIFO_NAME_H2C, O_WRONLY); 
            fd_read = open(FIFO_NAME_C2H, O_RDONLY);
        } else {
            fd_read = open(FIFO_NAME_H2C, O_RDONLY);
            fd_write = open(FIFO_NAME_C2H, O_WRONLY);
        }

        if (fd_read == -1 || fd_write == -1) {
            perror("fifo open");
            exit(1);
        }
    }

    ~Impl() {
        close(fd_read);
        close(fd_write);
        if (is_host) {
            unlink(FIFO_NAME_H2C);
            unlink(FIFO_NAME_C2H);
        }
    }
};

Conn::Conn(pid_t pid, bool host) : pImpl(new Impl(host)) {}
Conn::~Conn() { delete pImpl; }

bool Conn::Write(const ChatMessage& msg) {
    if (write(pImpl->fd_write, &msg, sizeof(msg)) == -1) return false;
    return true;
}

bool Conn::Read(ChatMessage& msg) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(pImpl->fd_read, &fds);

    struct timeval tv;
    tv.tv_sec = 1; 
    tv.tv_usec = 0;

    int ret = select(pImpl->fd_read + 1, &fds, nullptr, nullptr, &tv);
    if (ret > 0) {
        if (read(pImpl->fd_read, &msg, sizeof(msg)) > 0) return true;
    }
    return false;
}