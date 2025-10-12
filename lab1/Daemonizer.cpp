#include "Daemonizer.h"
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <syslog.h>

// Fully detach process and become a daemon. On failure logs and exits.
void becomeDaemon() {
    // Prevent child from becoming zombie
    signal(SIGCHLD, SIG_IGN);

    pid_t pid = fork();
    if (pid < 0) {
        // cannot log syslog here reliably if not opened, but attempt
        syslog(LOG_ERR, "daemonizer: first fork failed");
        exit(1);
    }
    if (pid > 0) {
        // parent exits
        exit(0);
    }

    if (setsid() < 0) {
        syslog(LOG_ERR, "daemonizer: setsid failed");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "daemonizer: second fork failed");
        exit(1);
    }
    if (pid > 0) {
        exit(0);
    }

    if (chdir("/") < 0) {
        syslog(LOG_ERR, "daemonizer: chdir / failed");
        exit(1);
    }
    umask(0);

    // Close all inherited fds
    long maxfd = sysconf(_SC_OPEN_MAX);
    if (maxfd == -1) maxfd = 1024;
    for (long fd = 0; fd < maxfd; ++fd) close(static_cast<int>(fd));

    // Reopen std fds to /dev/null
    int fd0 = open("/dev/null", O_RDWR);
    int fd1 = dup(fd0);
    int fd2 = dup(fd0);
    (void)fd1; (void)fd2;
}
