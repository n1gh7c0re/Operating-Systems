// Daemon.h
#ifndef DAEMON_H
#define DAEMON_H

#include <string>
#include <atomic>
#include "Config.h"

class Daemon {
private:
    static Daemon* instance;
    std::string configPath;
    Config currentConfig;
    std::atomic<bool> running;
    std::atomic<bool> reloadRequested;
    std::atomic<bool> terminateRequested;

    Daemon(const std::string& configPath);
    void processSignal(int sig);

public:
    static Daemon& getInstance(const std::string& configPath = "");
    static void signalHandler(int sig);
    void reloadConfig();
    void performTask();
    void run();
};

#endif // DAEMON_H