#pragma once
#include "ll/api/event/Cancellable.h"
#include "ll/api/event/Event.h"
#include "mc/world/actor/player/Player.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandPos.h"
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


// 玩家删除领地 (DeleteLandGui)
class PlayerDeleteLandBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player&    mPlayer;
    LandID     mLandID;
    int const& mRefundPrice; // 删除后返还的金额

public:
    constexpr explicit PlayerDeleteLandBeforeEvent(Player& player, LandID landID, int const& refundPrice)
    : Cancellable(),
      mPlayer(player),
      mLandID(landID),
      mRefundPrice(refundPrice) {}

    Player&    getPlayer() const;
    LandID     getLandID() const;
    int const& getRefundPrice() const;
};
class PlayerDeleteLandAfterEvent final : public ll::event::Event {
protected:
    Player& mPlayer;
    LandID  mLandID;

public:
    constexpr explicit PlayerDeleteLandAfterEvent(Player& player, LandID landID) : mPlayer(player), mLandID(landID) {}

    Player& getPlayer() const;
    LandID  getLandID() const;
};


// 领地成员变动(EditLandMemberGui)
class LandMemberChangeBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player&      mPlayer;       // 操作者
    UUIDs const& mTargetPlayer; // 目标玩家
    LandID       mLandID;
    bool         mIsAdd; // true: 添加成员, false: 删除成员

public:
    LandMemberChangeBeforeEvent(Player& player, UUIDs const& targetPlayer, LandID landID, bool isAdd)
    : Cancellable(),
      mPlayer(player),
      mTargetPlayer(targetPlayer),
      mLandID(landID),
      mIsAdd(isAdd) {}

    Player&      getPlayer() const;
    UUIDs const& getTargetPlayer() const;
    LandID       getLandID() const;
    bool         isAdd() const;
};
class LandMemberChangeAfterEvent final : public ll::event::Event {
protected:
    Player&      mPlayer;       // 操作者
    UUIDs const& mTargetPlayer; // 目标玩家
    LandID       mLandID;
    bool         mIsAdd; // true: 添加成员, false: 删除成员

public:
    LandMemberChangeAfterEvent(Player& player, UUIDs const& targetPlayer, LandID landID, bool isAdd)
    : mPlayer(player),
      mTargetPlayer(targetPlayer),
      mLandID(landID),
      mIsAdd(isAdd) {}

    Player&      getPlayer() const;
    UUIDs const& getTargetPlayer() const;
    LandID       getLandID() const;
    bool         isAdd() const;
};


// 领地主人变动(EditLandOwnerGui)
class LandOwnerChangeBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player& mPlayer;   // 操作者
    Player& mNewOwner; // 新主人
    LandID  mLandID;

public:
    LandOwnerChangeBeforeEvent(Player& player, Player& newOwner, LandID landID)
    : Cancellable(),
      mPlayer(player),
      mNewOwner(newOwner),
      mLandID(landID) {}

    Player& getPlayer() const;
    Player& getNewOwner() const;
    LandID  getLandID() const;
};
class LandOwnerChangeAfterEvent final : public ll::event::Event {
protected:
    Player& mPlayer;   // 操作者
    Player& mNewOwner; // 目标玩家
    LandID  mLandID;

public:
    LandOwnerChangeAfterEvent(Player& player, Player& newOwner, LandID landID)
    : mPlayer(player),
      mNewOwner(newOwner),
      mLandID(landID) {}

    Player& getPlayer() const;
    Player& getNewOwner() const;
    LandID  getLandID() const;
};


// 领地范围变动(LandBuyWithReSelectGui)
class LandRangeChangeBeforeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Player&            mPlayer;      // 操作者
    LandDataPtr const& mLandData;    // 操作的领地数据
    LandPos const&     mNewRange;    // 新范围
    int const&         mNeedPay;     // 需要支付的价格
    int const&         mRefundPrice; // 需要退的价格

public:
    LandRangeChangeBeforeEvent(
        Player&            player,
        LandDataPtr const& landData,
        LandPos const&     newRange,
        int const&         needPay,
        int const&         refundPrice
    )
    : Cancellable(),
      mPlayer(player),
      mLandData(landData),
      mNewRange(newRange),
      mNeedPay(needPay),
      mRefundPrice(refundPrice) {}

    Player&            getPlayer() const;
    LandDataPtr const& getLandData() const;
    LandPos const&     getNewRange() const;
    int const&         getNeedPay() const;
    int const&         getRefundPrice() const;
};
class LandRangeChangeAfterEvent final : public ll::event::Event {
protected:
    Player&            mPlayer;      // 操作者
    LandDataPtr const& mLandData;    // 操作的领地数据
    LandPos const&     mNewRange;    // 新范围
    int const&         mNeedPay;     // 需要支付的价格
    int const&         mRefundPrice; // 需要退的价格

public:
    LandRangeChangeAfterEvent(
        Player&            player,
        LandDataPtr const& landData,
        LandPos const&     newRange,
        int const&         needPay,
        int const&         refundPrice
    )
    : mPlayer(player),
      mLandData(landData),
      mNewRange(newRange),
      mNeedPay(needPay),
      mRefundPrice(refundPrice) {}

    Player&            getPlayer() const;
    LandDataPtr const& getLandData() const;
    LandPos const&     getNewRange() const;
    int const&         getNeedPay() const;
    int const&         getRefundPrice() const;
};


} // namespace land