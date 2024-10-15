#pragma once
#include "Global.h"
#include "mc/world/actor/player/Player.h"
#include "pLand/Particle.h"
#include "pland/LandData.h"
#include <memory>
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

    // static std::unordered_map<UUIDs, std::vector<std::weak_ptr<LandData>>> mDrawDatas; // 绘制数据

    LDAPI static void setup();   // 初始化
    LDAPI static void release(); // 释放资源
};


} // namespace land