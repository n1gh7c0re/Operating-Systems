#include "conn.h"
#include <atomic>

std::atomic<bool> running{true};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <HOST_PID>" << std::endl;
        return 1;
    }

    pid_t host_pid = std::stoi(argv[1]);
    Log("CLIENT", "Started. Sending signal to host " + std::to_string(host_pid));

    if (kill(host_pid, SIGUSR1) == -1) {
        perror("kill");
        return 1;
    }

    sleep(1);

    Conn connection(host_pid, false);

    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    Log("CLIENT", "Connected. Chat started.");

    while (running) {
        // 1. Читаем ввод с клавиатуры
        char buf[MSG_SIZE];
        if (read(STDIN_FILENO, buf, sizeof(buf)) > 0) {
            buf[strcspn(buf, "\n")] = 0; // Убираем enter
            
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
                break; // Выход из основного цикла
            }
            std::string senderName = (inMsg.sender_pid == getpid()) ? "Me" : "Partner";
            
            if (inMsg.type == MSG_PRIVATE) {
                Log(senderName + " [PRIVATE]", inMsg.content);
            } else {
                Log(senderName, inMsg.content);
            }
        }

        usleep(50000);
    }

    return 0;
}