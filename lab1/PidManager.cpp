#include "PidManager.h"
#include <fstream>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <string>

void checkAndTerminateOldPid(const std::string& pidFile) {
    std::ifstream pidIn(pidFile);
    if (pidIn.good()) {
        pid_t oldPid;
        pidIn >> oldPid;
        pidIn.close();
        if (oldPid > 0) {
            std::string procPath = "/proc/" + std::to_string(oldPid);
            struct stat st;
            if (stat(procPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                kill(oldPid, SIGTERM);
                sleep(2);
            }
        }
        remove(pidFile.c_str());
    }
}

void writePid(const std::string& pidFile) {
    std::ofstream pidOut(pidFile);
    if (pidOut.is_open()) {
        pidOut << getpid();
        pidOut.close();
    }
}