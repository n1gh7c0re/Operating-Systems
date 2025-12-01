#ifndef PID_MANAGER_H
#define PID_MANAGER_H

#include <string>

void checkAndTerminateOldPid(const std::string& pidFile);
void writePid(const std::string& pidFile);

#endif // PID_MANAGER_H