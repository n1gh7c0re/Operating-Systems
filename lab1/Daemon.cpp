#include "Daemon.h"
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <chrono>
#include <thread>

Daemon* Daemon::instance = nullptr;

Daemon::Daemon(const std::string& configPath)
    : configPath(configPath), running(false),
      reloadRequested(false), terminateRequested(false) {}

Daemon& Daemon::getInstance(const std::string& configPath) {
    if (instance == nullptr) {
        if (configPath.empty()) {
            throw std::runtime_error("Config path required");
        }
        instance = new Daemon(configPath);
    }
    return *instance;
}

void Daemon::signalHandler(int sig) {
    if (instance != nullptr) {
        instance->processSignal(sig);
    }
}

void Daemon::processSignal(int sig) {
    if (sig == SIGHUP) {
        reloadRequested.store(true);
    } else if (sig == SIGTERM) {
        terminateRequested.store(true);
    }
}

void Daemon::reloadConfig() {
    currentConfig = loadConfig(configPath);
    syslog(LOG_NOTICE, "Config reloaded: src_dir=%s, dest_dir=%s, interval=%d",
           currentConfig.srcDir.c_str(), currentConfig.destDir.c_str(), currentConfig.interval);
}

void Daemon::performTask() {
    if (currentConfig.srcDir.empty() || currentConfig.destDir.empty()) {
        syslog(LOG_ERR, "Source or destination directory not set");
        return;
    }
    DIR* dir = opendir(currentConfig.srcDir.c_str());
    if (!dir) {
        syslog(LOG_ERR, "Could not open source directory %s", currentConfig.srcDir.c_str());
        return;
    }
    std::string totalPath = currentConfig.destDir + "/total.log";
    std::ofstream outfile(totalPath, std::ios::app);
    if (!outfile.is_open()) {
        syslog(LOG_ERR, "Could not open target file %s", totalPath.c_str());
        closedir(dir);
        return;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        if (name.size() >= 4 && name.substr(name.size() - 4) == ".log") {
            std::string srcPath = currentConfig.srcDir + "/" + name;
            std::ifstream infile(srcPath);
            if (!infile.is_open()) {
                syslog(LOG_ERR, "Could not open source file %s", srcPath.c_str());
                continue;
            }
            outfile << "\n\n" << name << "\n\n" << infile.rdbuf();
            infile.close();
            outfile.flush();
            if (remove(srcPath.c_str()) != 0) {
                syslog(LOG_ERR, "Could not remove source file %s", srcPath.c_str());
            } else {
                syslog(LOG_INFO, "Processed and removed file %s", srcPath.c_str());
            }
        }
    }
    closedir(dir);
    outfile.close();
}

void Daemon::run() {
    signal(SIGHUP, Daemon::signalHandler);
    signal(SIGTERM, Daemon::signalHandler);
    openlog("mydaemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "Daemon started with PID %d", getpid());
    reloadConfig();
    running.store(true);
    while (running.load()) {
        if (reloadRequested.load()) {
            reloadConfig();
            reloadRequested.store(false);
        }
        performTask();
        for (int i = 0; i < currentConfig.interval && running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (reloadRequested.load() || terminateRequested.load()) break;
        }
        if (terminateRequested.load()) {
            running.store(false);
        }
    }
    syslog(LOG_NOTICE, "Daemon exiting");
    closelog();
    std::string pidFile;
    auto pos = configPath.find_last_of('/');
    if (pos != std::string::npos) {
        pidFile = configPath.substr(0, pos) + "/daemon.pid";
    } else {
        pidFile = "daemon.pid";
    }
    remove(pidFile.c_str());
}