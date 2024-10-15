#pragma once
#include "Global.h"
#include "mc/world/actor/player/Player.h"
#include "pLand/Particle.h"
#include "pland/LandData.h"
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>


namespace land {


class LandDraw {
public:
    LandDraw()                           = delete;
    LandDraw(LandDraw&&)                 = delete;
    LandDraw(const LandDraw&)            = delete;
    LandDraw& operator=(LandDraw&&)      = delete;
    LandDraw& operator=(const LandDraw&) = delete;

    //                        uuid               cur / near           particles
    LDAPI static std::unordered_map<UUIDm, std::pair<bool, std::vector<Particle>>> mDrawList;
    LDAPI static std::jthread                                                      mThread;

    LDAPI static void setup();   // 初始化
    LDAPI static void release(); // 释放资源

    LDAPI static void disable();
    LDAPI static void disable(Player& player);
    LDAPI static void enable(Player& player, bool onlyDrawCurrentLand = true); // 开启绘制 (默认只绘制当前区块)
};


} // namespace land