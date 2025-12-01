#include "Config.h"
#include <fstream>
#include <syslog.h>

Config loadConfig(const std::string& path) {
    Config conf = {"", "", 60};
    std::ifstream file(path);
    if (!file.is_open()) {
        syslog(LOG_ERR, "Could not open config file %s", path.c_str());
        return conf;
    }
    std::string configDir;
    auto pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        configDir = path.substr(0, pos);
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;
        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);
        if (key == "src_dir") {
            if (!value.empty() && value[0] != '/') {
                value = configDir + "/" + value;
            }
            conf.srcDir = value;
        } else if (key == "dest_dir") {
            if (!value.empty() && value[0] != '/') {
                value = configDir + "/" + value;
            }
            conf.destDir = value;
        } else if (key == "interval") {
            try {
                conf.interval = std::stoi(value);
                if (conf.interval <= 0) conf.interval = 60;
            } catch (...) {
                syslog(LOG_ERR, "Invalid interval in config: %s", value.c_str());
            }
        }
    }
    file.close();
    if (conf.srcDir.empty() || conf.destDir.empty()) {
        syslog(LOG_ERR, "Config missing src_dir or dest_dir");
    }
    return conf;
}