#include "PLand/Config.h"
#include "ll/api/Config.h"
#include "mod/MyMod.h"
#include <filesystem>


namespace land {

namespace fs = std::filesystem;
#define CONFIG_FILE "Config.json"

bool Config::tryLoad() {
    auto dir = my_mod::MyMod::getInstance().getSelf().getConfigDir() / CONFIG_FILE;

    if (!fs::exists(dir)) {
        trySave();
    }

    bool status = ll::config::loadConfig(Config::cfg, dir);

    return status ? status : tryUpdate();
}

bool Config::trySave() {
    auto dir = my_mod::MyMod::getInstance().getSelf().getConfigDir() / CONFIG_FILE;

    bool status = ll::config::saveConfig(cfg, dir);

    return status;
}

bool Config::tryUpdate() { return trySave(); }

Config::Data Config::cfg; // static

} // namespace land