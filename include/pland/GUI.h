#pragma once
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/wrapper/FormEx.h"
#include <functional>

namespace land {
using namespace wrapper;

// 独立GUI
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


// 通用选择GUI
class ChooseLandGui {
public:
    using ChooseCallback = std::function<void(Player&, LandID id)>;
    template <typename ParentForm = void>
    static void impl(Player& player, ChooseCallback callback);
};
class ChoosePlayerGui {
public:
    using ChoosePlayerCall = std::function<void(Player&, Player& choosedPlayer)>;
    template <typename ParentForm = void>
    static void impl(Player& player, ChoosePlayerCall callback);
};


// 主界面
class LandMainGui {
public:
    static void impl(Player& player);
};


// 领地管理GUI
class LandManagerGui {
public:
    static void impl(Player& player, LandID id);

    class EditLandPermGui {
    public:
        static void impl(Player& player, LandDataPtr ptr);
    };

    class DeleteLandGui {
    public:
        static void impl(Player& player, LandDataPtr ptr);
    };
};


// 领地成员管理GUI
class EditLandMemberGui {
public:
    static void impl(Player& player, LandDataPtr ptr);

    class AddMemberGui {
    public:
        static void impl(Player& player, LandDataPtr ptr);
    };

    class RemoveMemberGui {
    public:
        static void impl(Player& player, LandDataPtr ptr, UUIDs members);
    };
};


// 领地传送GUI
class LandTeleportGui {
public:
    static void impl(Player& player, LandID id);
};


// 管理员GUI
class LandOPManagerGui {
public:
    static void impl(Player& player);
};

} // namespace land