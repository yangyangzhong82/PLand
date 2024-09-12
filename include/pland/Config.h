#pragma once
#include "Global.h"
#include <vector>

namespace land {

enum class EconomyKit : int { LegacyMoney, ScoreBoard };

struct Config {
    int version{1};
    int logLevel{4};

    struct {
        bool       enabled{true};                // 是否启用
        EconomyKit kit{EconomyKit::LegacyMoney}; // 经济系统
        string     economyName{"money"};         // 经济名称
        string     scoreboardObjName;            // 计分板对象名称
    } economy;

    struct {
        bool   landTp{true};    // 领地传送
        int    maxLand{20};     // 最大领地数量
        int    minSpacing{16};  // 最小领地间距
        double refundRate{0.9}; // 退款率(1为全额退款)
        double discountRate{1}; // 折扣率(1为无折扣)

        struct {
            bool enterTip{true};           // 进入领地提示
            bool bottomContinuedTip{true}; // 底部持续提示
            int  bottomTipFrequency{1};    // 底部提示频率(s)
        } tip;

        // 购买配置
        struct {
            struct {
                bool   enabled{true};
                string calculate{"square * 25"}; // 计算公式
            } threeDimensionl;

            struct {
                bool   enabled{true};
                string calculate{"square * 8 + height * 20"}; // 计算公式
            } twoDimensionl;

            struct {
                int min{4};     // 最小领地范围
                int max{60000}; // 最大领地范围
            } squareRange;

            std::vector<LandDimid> allowDimensions{0, 1, 2}; // 允许的领地维度
        } bought;
    } land;

    struct {
        bool   drawParticle{true};                   // 是否绘制粒子效果
        string particle{"minecraft:villager_happy"}; // 粒子效果
        string tool{"minecraft:stick"};              // 工具
    } selector;

    // Functions
    LDAPI static Config cfg;
    LDAPI static bool   tryLoad();
    LDAPI static bool   trySave();
    LDAPI static bool   tryUpdate();
};


} // namespace land
