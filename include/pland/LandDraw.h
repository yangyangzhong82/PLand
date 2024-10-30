#pragma once
#include "Global.h"
#include "mc/world/actor/player/Player.h"
#include "pLand/Particle.h"
#include "pland/LandData.h"
#include <memory>
#include <mutex>
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

    enum class DrawType : int { Disable = 0, CurrentLand, NearLand };

    LDAPI static std::unordered_map<UUIDm, DrawType> mDrawType;

    LDAPI static void setup();   // 初始化
    LDAPI static void release(); // 释放资源

    LDAPI static void disable();
    LDAPI static void disable(Player& player);
    LDAPI static void enable(Player& player, DrawType type = DrawType::CurrentLand); // 开启绘制 (默认只绘制当前区块)
};


} // namespace land