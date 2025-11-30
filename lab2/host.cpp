#include "conn.h"
#include <vector>
#include <atomic>

std::atomic<pid_t> client_pid_global{0};
std::atomic<bool> connection_established{false};

void signal_handler(int sig, siginfo_t *si, void *ucontext) {
    if (sig == SIGUSR1) {
        client_pid_global = si->si_pid;
        connection_established = true;
    }
}

int main() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = signal_handler;
    if (sigaction(SIGUSR1, &sa, nullptr) == -1) {
        perror("sigaction");
        return 1;
    }

    Log("HOST", "Started. PID: " + std::to_string(getpid()));
    Log("HOST", "Waiting for client signal (SIGUSR1)...");

    while (!connection_established) {
        usleep(100000);
    }

    Log("HOST", "Client connected! PID: " + std::to_string(client_pid_global));

    Conn connection(client_pid_global, true);

    time_t last_client_activity = time(nullptr);
    
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    Log("HOST", "Chat started. Type messages.");

    while (true) {
        // 1. Читаем ввод с клавиатуры
        char buf[MSG_SIZE];
        if (read(STDIN_FILENO, buf, sizeof(buf)) > 0) {
            buf[strcspn(buf, "\n")] = 0; // Убираем enter
            
            // --- ПРОВЕРКА НА КОМАНДУ ВЫХОДА ---
            if (strcmp(buf, "/quit") == 0 || strcmp(buf, "/exit") == 0) {
                ChatMessage disconnectMsg;
                disconnectMsg.mtype = 1;
                disconnectMsg.sender_pid = getpid();
                disconnectMsg.timestamp = time(nullptr);
                disconnectMsg.type = MSG_DISCONNECT; 
                strcpy(disconnectMsg.content, "Session terminated by sender.");

                connection.Write(disconnectMsg);
                Log("CONTROL", "Sending disconnect signal and shutting down.");
                break; 
            }

            if (strlen(buf) > 0) {
                ChatMessage msg;
                msg.mtype = 1;
                msg.sender_pid = getpid();
                msg.timestamp = time(nullptr);
                
                std::string text(buf);
                if (text.find("@private") == 0) {
                    msg.type = MSG_PRIVATE;
                    size_t offset = (text.length() > 8 && text[8] == ' ') ? 9 : 8;
                    strncpy(msg.content, buf + offset, MSG_SIZE);
                    Log("Me (Private)", msg.content);
                } else {
                    msg.type = MSG_GENERAL;
                    strncpy(msg.content, buf, MSG_SIZE);
                    Log("Me", buf);
                }

                if (!connection.Write(msg)) {
                    Log("ERROR", "Failed to send message");
                }
            }
        }

        // 2. Читаем из сети
        ChatMessage inMsg;
        if (connection.Read(inMsg)) {
            if (inMsg.type == MSG_DISCONNECT) {
                Log("CONTROL", "Partner requested disconnection. Shutting down.");
                break; 
            }
            last_client_activity = time(nullptr);

            std::string senderName = (inMsg.sender_pid == getpid()) ? "Me" : "Partner"; // В 1-to-1 PID партнера очевиден
            
            if (inMsg.type == MSG_PRIVATE) {
                Log(senderName + " [PRIVATE]", inMsg.content);
            } else {
                Log(senderName, inMsg.content);
            }
        }

        // 3. Проверка таймаута неактивности клиента
        if (IsTimeout(last_client_activity)) {
            Log("HOST", "Client inactivity timeout (>1 min). Killing client.");
            kill(client_pid_global, SIGKILL);
            break;
        }

        usleep(50000); // Чтобы не грузить CPU
    }

    return 0;
}