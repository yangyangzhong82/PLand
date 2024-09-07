#pragma once
#include "ll/api/event/Cancellable.h"
#include "ll/api/event/Event.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandSelector.h"

namespace land {


// 玩家请求创建领地 (ChooseLandDimAndNewLand)
class PlayerAskCreateLandBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player& mPlayer;

public:
    constexpr explicit PlayerAskCreateLandBeforeEvent(Player& player) : Cancellable(), mPlayer(player) {}

    Player& getPlayer() const;
};
class PlayerAskCreateLandAfterEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    bool    mIs3DLand;

public:
    constexpr explicit PlayerAskCreateLandAfterEvent(Player& player, bool is3DLand)
    : mPlayer(player),
      mIs3DLand(is3DLand) {}

    Player& getPlayer() const;
    bool    is3DLand() const;
};


// 玩家购买领地 (LandBuyGui)
class PlayerBuyLandBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player&           mPlayer;
    LandSelectorData* mLandSelectorData;
    int&              mPrice;

public:
    constexpr explicit PlayerBuyLandBeforeEvent(Player& player, LandSelectorData* landSelectorData, int& price)
    : Cancellable(),
      mPlayer(player),
      mLandSelectorData(landSelectorData),
      mPrice(price) {}

    Player&           getPlayer() const;
    LandSelectorData* getLandSelectorData() const;
    int&              getPrice() const;
};
class PlayerBuyLandAfterEvent final : public ll::event::Event {
protected:
    Player&     mPlayer;
    LandDataPtr mLandData;

public:
    explicit PlayerBuyLandAfterEvent(Player& player, LandDataPtr landData) : mPlayer(player), mLandData(landData) {}

    Player&     getPlayer() const;
    LandDataPtr getLandData() const;
};


// 玩家 进入/离开 领地(LandScheduler)
class PlayerEnterLandEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    LandID  mLandID;

public:
    constexpr explicit PlayerEnterLandEvent(Player& player, LandID landID) : mPlayer(player), mLandID(landID) {}

    Player& getPlayer() const;
    LandID  getLandID() const;
};
class PlayerLeaveLandEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    LandID  mLandID;

public:
    constexpr explicit PlayerLeaveLandEvent(Player& player, LandID landID) : mPlayer(player), mLandID(landID) {}

    Player& getPlayer() const;
    LandID  getLandID() const;
};


} // namespace land