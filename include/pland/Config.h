#pragma once

namespace land {


class Config {
    Config()                         = delete;
    Config(Config&&)                 = delete;
    Config(const Config&)            = delete;
    Config& operator=(const Config&) = delete;
    Config& operator=(Config&&)      = delete;

    static struct Data {
        int version{1};
    } cfg;

    static bool tryLoad();

    static bool trySave();

    static bool tryUpdate();
};


} // namespace land
