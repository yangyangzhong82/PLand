#pragma once
#include "mc/world/actor/player/Player.h"

namespace land {


// 选择领地维度(2D/3D)
class ChooseLandDimidensionlAndNew {
public:
    static void send(Player& player);
};


// 领地GUI入口
class LandMainGui {
public:
    static void send(Player& player);
};

class LandManagerGui {
public:
    static void send(Player& player);
};

class LandBuyGui {
public:
    static void send(Player& player);
};


} // namespace land