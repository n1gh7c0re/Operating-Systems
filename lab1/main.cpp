#include "Daemon.h"
#include "PidManager.h"
#include "Daemonizer.h"
#include <iostream>
#include <limits.h>
#include <string>

int main(int argc, char* argv[]) {
    std::string configFile = "config.txt";
    if (argc > 1) {
        configFile = argv[1];
    }
    char realPath[PATH_MAX];
    if (realpath(configFile.c_str(), realPath) == nullptr) {
        std::cerr << "Error: unable to resolve config file path" << std::endl;
        return 1;
    }
    std::string configPath(realPath);
    auto pos = configPath.find_last_of('/');
    std::string baseDir = (pos != std::string::npos) ? configPath.substr(0, pos) : ".";
    std::string pidFile = baseDir + "/daemon.pid";

    checkAndTerminateOldPid(pidFile);
    becomeDaemon();
    writePid(pidFile);

    try {
        Daemon& daemon = Daemon::getInstance(configPath);
        daemon.run();
    } catch (const std::exception& ex) {
        return 1;
    }
    return 0;
}