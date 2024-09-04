#pragma once
#include "mc/world/actor/player/Player.h"
#include "pland/wrapper/FormEx.h"

namespace land {
using namespace wrapper;


class LandMainGui : public FormWrapper<LandMainGui> {
public:
    static void impl(Player& player);
};

class ChooseLandDimAndNewLand : public FormWrapper<ChooseLandDimAndNewLand> {
public:
    static void impl(Player& player);
};

class LandManagerGui : public FormWrapper<LandManagerGui> {
public:
    static void impl(Player& player);
};

class LandBuyGui : public FormWrapper<LandBuyGui> {
public:
    static void impl(Player& player);
};


} // namespace land