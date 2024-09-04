#pragma once
#include "Global.h"
#include "mc/world/actor/player/Player.h"
#include "utils/Utils.h"


namespace land {

class EconomySystem {
public:
    EconomySystem()                                = default;
    EconomySystem(const EconomySystem&)            = delete;
    EconomySystem& operator=(const EconomySystem&) = delete;

    static EconomySystem& getInstance();

    long long get(Player& player);

    bool set(Player& player, long long money);

    bool add(Player& player, long long money);

    bool reduce(Player& player, long long money);

    string getSpendTip(Player& player, long long money);

    void sendLackMoneyTip(Player& player, long long money); // 发送经济不足提示
};


} // namespace land