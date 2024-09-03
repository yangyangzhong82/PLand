#pragma once
#include <string>

namespace land {

using string = std::string;


struct Config {
    int version{1};

    struct {
        string particle{"minecraft:villager_happy"};
        string tool{"minecraft:stick"};
    } selector;


    // Functions
    static Config cfg;
    static bool   tryLoad();
    static bool   trySave();
    static bool   tryUpdate();
};


} // namespace land
