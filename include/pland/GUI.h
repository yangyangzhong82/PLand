#pragma once
#include "mc/world/actor/player/Player.h"
#include "pland/wrapper/FormEx.h"

namespace land {
using namespace wrapper;

// 插件主界面
class LandMainGui : public FormWrapper<LandMainGui> {
public:
    static void impl(Player& player);
};

// 领地购买GUI
class LandBuyGui : public FormWrapper<LandBuyGui> {
public:
    static void impl(Player& player);
};

// 选择领地维度并创建新领地
class ChooseLandDimAndNewLand : public FormWrapper<ChooseLandDimAndNewLand> {
public:
    static void impl(Player& player);
};

// 选区完成修改Y范围
class SelectorChangeYGui : public FormWrapper<SelectorChangeYGui> {
public:
    static void impl(Player& player);
};

// 继续Gui
class ContinueGui : public FormWrapper<ContinueGui> {
public:
    static void impl(Player& player);
};


// TODO:
class LandOPManagerGui : public FormWrapper<LandOPManagerGui> {
public:
    static void impl(Player& player);
};

} // namespace land