#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    std::string srcDir;
    std::string destDir;
    int interval;
};

Config loadConfig(const std::string& path);

#endif // CONFIG_H