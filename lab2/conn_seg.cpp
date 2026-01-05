#include "conn.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>


struct ShmLayout {
    ChatMessage host_slot;   
    bool host_has_data;      
    
    ChatMessage client_slot; 
    bool client_has_data;
};

struct Conn::Impl {
    int shmid;
    ShmLayout* mem;
    sem_t* sem_mutex; // Глобальный семафор для атомарного доступа
    bool is_host;

    Impl(bool host) : is_host(host) {
        key_t key = ftok(SHM_KEY_FILE, SHM_KEY_PROJ_ID);
        if (key == -1) { perror("ftok"); exit(1); }

        if (is_host) {
            shmid = shmget(key, sizeof(ShmLayout), 0666 | IPC_CREAT);
        } else {
            shmid = shmget(key, sizeof(ShmLayout), 0666);
        }
        
        if (shmid == -1) { perror("shmget"); exit(1); }

        mem = (ShmLayout*)shmat(shmid, nullptr, 0);
        if (mem == (void*)-1) { perror("shmat"); exit(1); }

        sem_mutex = sem_open(SEM_NAME_HOST, O_CREAT, 0666, 1);
        if (sem_mutex == SEM_FAILED) { perror("sem_open"); exit(1); }

        if (is_host) {
            WaitSem();
            mem->host_has_data = false;
            mem->client_has_data = false;
            PostSem();
        }
    }

    ~Impl() {
        shmdt(mem);
        sem_close(sem_mutex);
        if (is_host) {
            shmctl(shmid, IPC_RMID, nullptr);
            sem_unlink(SEM_NAME_HOST);
        }
    }

    void WaitSem() {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += TIMEOUT_SEC;
        if (sem_timedwait(sem_mutex, &ts) == -1) {
            Log("IPC", "Semaphore timeout or error");
        }
    }

    void PostSem() {
        sem_post(sem_mutex);
    }
};

Conn::Conn(pid_t pid, bool host) : pImpl(new Impl(host)) {}
Conn::~Conn() { delete pImpl; }

bool Conn::Write(const ChatMessage& msg) {
    pImpl->WaitSem();
    bool success = false;
    
    if (pImpl->is_host) {
        if (!pImpl->mem->host_has_data) {
            pImpl->mem->host_slot = msg;
            pImpl->mem->host_has_data = true;
            success = true;
        }
    } else {
        if (!pImpl->mem->client_has_data) {
            pImpl->mem->client_slot = msg;
            pImpl->mem->client_has_data = true;
            success = true;
        }
    }
    
    pImpl->PostSem();
    return success;
}

bool Conn::Read(ChatMessage& msg) {
    pImpl->WaitSem();
    bool success = false;

    if (pImpl->is_host) {
        if (pImpl->mem->client_has_data) {
            msg = pImpl->mem->client_slot;
            pImpl->mem->client_has_data = false; 
            success = true;
        }
    } else {
        if (pImpl->mem->host_has_data) {
            msg = pImpl->mem->host_slot;
            pImpl->mem->host_has_data = false;
            success = true;
        }
    }

    pImpl->PostSem();
    return success;
}