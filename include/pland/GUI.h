#pragma once
#include "mc/world/actor/player/Player.h"
#include "pland/wrapper/FormEx.h"

namespace land {
using namespace wrapper;


class LandMainGui : public FormWrapper<LandMainGui> {
public:
    static void impl(Player& player);
};

class LandBuyGui : public FormWrapper<LandBuyGui, LandMainGui> {
public:
    static void impl(Player& player);
};

class ChooseLandDimAndNewLand : public FormWrapper<ChooseLandDimAndNewLand> {
public:
    static void impl(Player& player);
};


// 选区完成修改Y范围
class SelectorChangeYGui : public FormWrapper<SelectorChangeYGui> {
public:
    static void impl(Player& player);
};


// TODO:
class LandManagerGui : public FormWrapper<LandManagerGui> {
public:
    static void impl(Player& player);
};

} // namespace land