#pragma once
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/wrapper/FormEx.h"
#include <functional>

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

// 玩家选择领地
class ChooseLandGui : public FormWrapper<ChooseLandGui, void, void, SimpleFormExBack::Upper> {
public:
    using ChooseCallback = std::function<void(Player&, LandID id)>;
    template <typename DynamicParentForm = void, typename DynamicParentCall = void>
    static void impl(Player& player, ChooseCallback callback);
};

class LandManagerGui : public FormWrapper<LandManagerGui> {
public:
    static void impl(Player& player, LandID);
};

class EditLandPermGui : public FormWrapper<EditLandPermGui> {
public:
    static void impl(Player& player, LandDataPtr ptr);
};

class DeleteLandGui : public FormWrapper<DeleteLandGui> {
public:
    static void impl(Player& player, LandDataPtr ptr);
};

class EditLandMemberGui : public FormWrapper<EditLandMemberGui> {
public:
    static void impl(Player& player, LandDataPtr ptr);
};

class LandTeleportGui : public FormWrapper<LandTeleportGui> {
public:
    static void impl(Player& player);
};


// TODO:
class LandOPManagerGui : public FormWrapper<LandOPManagerGui> {
public:
    static void impl(Player& player);
};

} // namespace land