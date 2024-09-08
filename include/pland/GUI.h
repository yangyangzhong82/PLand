#pragma once
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/wrapper/FormEx.h"
#include <functional>

namespace land {
using namespace wrapper;

class LandMainGui {
public:
    static void impl(Player& player);
};

class LandBuyGui {
public:
    static void impl(Player& player);
};

class ChooseLandDimAndNewLand {
public:
    static void impl(Player& player);
};

class SelectorChangeYGui {
public:
    static void impl(Player& player);
};

class ChooseLandGui {
public:
    using ChooseCallback = std::function<void(Player&, LandID id)>;
    template <typename ParentForm = void>
    static void impl(Player& player, ChooseCallback callback);
};

class LandManagerGui {
public:
    static void impl(Player& player, LandID id);
};

class EditLandPermGui {
public:
    static void impl(Player& player, LandDataPtr ptr);
};

class DeleteLandGui {
public:
    static void impl(Player& player, LandDataPtr ptr);
};

class EditLandMemberGui {
public:
    static void impl(Player& player, LandDataPtr ptr);
};

class LandTeleportGui {
public:
    static void impl(Player& player, LandID id);
};


class LandOPManagerGui {
public:
    static void impl(Player& player);
};

} // namespace land