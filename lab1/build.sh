#!/bin/bash
set -e
g++ -std=c++11 -Wall -Werror -c Config.cpp
g++ -std=c++11 -Wall -Werror -c PidManager.cpp
g++ -std=c++11 -Wall -Werror -c Daemonizer.cpp
g++ -std=c++11 -Wall -Werror -c Daemon.cpp
g++ -std=c++11 -Wall -Werror -c main.cpp
g++ -std=c++11 -o mydaemon Config.o PidManager.o Daemonizer.o Daemon.o main.o
rm -f *.o
echo "Build complete. Executable: mydaemon"