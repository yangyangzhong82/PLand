#include "pland/LandSelector.h"
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/item/ItemStack.h"
#include "mod/MyMod.h"
#include "pland/Config.h"
#include "pland/DrawHandleManager.h"
#include "pland/GUI.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/utils/Date.h"
#include "pland/utils/McUtils.h"
#include <memory>
#include <optional>
#include <unordered_map>


#include "mc/network/packet/SetTitlePacket.h"

namespace land {


LandSelector& LandSelector::getInstance() {
    static LandSelector instance;
    return instance;
}

ll::event::ListenerPtr                 mPlayerUseItemOn;
std::unordered_map<UUIDm, std::time_t> mStabilized; // 防抖
bool                                   LandSelector::init() {
    auto& bus = ll::event::EventBus::getInstance();
    mPlayerUseItemOn =
        bus.emplaceListener<ll::event::PlayerInteractBlockEvent>([this](ll::event::PlayerInteractBlockEvent const& ev) {
            auto& pl = ev.self();

            if (auto iter = mStabilized.find(pl.getUuid()); iter != mStabilized.end()) {
                if (iter->second >= Date::now().getTime()) {
                    return;
                }
            }
            mStabilized[pl.getUuid()] = Date::future(50 / 1000).getTime(); // 50ms

            if (this->isSelected(pl)) {
                mc_utils::executeCommand("pland buy", &pl);
                return;
            }

            if (!this->isSelectTool(ev.item()) || !this->isSelecting(pl)) {
                return;
            }

#ifdef DEBUG
            my_mod::MyMod::getInstance().getSelf().getLogger().info(
                "PlayerInteractBlockEvent: {}",
                ev.blockPos().toString()
            );
#endif

            if (!this->isSelectedPointA(pl)) {
                if (this->trySelectPointA(pl, ev.blockPos())) {
                    mc_utils::sendText(pl, "已选择点A \"{}\""_trf(pl, ev.blockPos().toString()));
                } else {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "选择失败"_trf(pl));
                }

            } else if (!this->isSelectedPointB(pl) && this->isSelectedPointA(pl)) {
                if (this->trySelectPointB(pl, ev.blockPos())) {
                    mc_utils::sendText(pl, "已选择点B \"{}\""_trf(pl, ev.blockPos().toString()));


                    if (auto iter = mSelectors.find(pl.getUuid()); iter != mSelectors.end()) {
                        auto& data = iter->second;
                        if (data->mDraw3D) {
                            // 3DLand
                            SelectorChangeYGui::impl(pl); // 发送GUI
                        } else {
                            // 2DLand
                            auto dim = pl.getLevel().getDimension(data->mDimid);
                            if (auto lock = dim.lock(); lock) {
                                data->mPos.mMax_B.y = mc_utils::GetDimensionMaxHeight(*lock);
                                data->mPos.mMin_A.y = mc_utils::GetDimensionMinHeight(*lock);
                            } else {
                                mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "获取维度失败"_trf(pl));
                            }
                        }
                    }
                } else {
                    mc_utils::sendText<mc_utils::LogLevel::Error>(pl, "选择失败"_trf(pl));
                }
            }
        });

    using ll::chrono_literals::operator""_tick; // 1s = 20_tick
    // repeat task
    ll::coro::keepThis([this]() -> ll::coro::CoroTask<> {
        while (GlobalRepeatCoroTaskRunning) {
            co_await 20_tick;
            if (!GlobalRepeatCoroTaskRunning) co_return;
            for (auto& [uuid, data] : mSelectors) {
                try {
                    auto pl = data->mPlayer; // 玩家指针
                    if (pl == nullptr) {
                        mSelectors.erase(uuid); // 玩家断开连接
                        continue;
                    }

                    SetTitlePacket titlePacket(SetTitlePacket::TitleType::Title);
                    SetTitlePacket subTitlePacket(SetTitlePacket::TitleType::Subtitle);
                    if (!data->mSelectedPointA) {
                        titlePacket.mTitleText = "[ 选区器 ]"_trf(*pl);
                        subTitlePacket.mTitleText =
                            "使用 /pland set a 或使用 {} 选择点A"_trf(*pl, Config::cfg.selector.tool);

                    } else if (!data->mSelectedPointB) {
                        titlePacket.mTitleText = "[ 选区器 ]"_trf(*pl);
                        subTitlePacket.mTitleText =
                            "使用 /pland set b 或使用 {} 选择点B"_trf(*pl, Config::cfg.selector.tool);

                    } else if (data->mCanDraw && Config::cfg.selector.drawParticle) {
                        titlePacket.mTitleText    = "[ 选区完成 ]"_trf(*pl);
                        subTitlePacket.mTitleText = "使用 /pland buy 呼出购买菜单"_trf(*pl, Config::cfg.selector.tool);

                        // 绘制粒子
                        if (!data->mIsDrawedBox) {
                            data->mIsDrawedBox    = true;
                            data->mDrawedBoxGeoId = DrawHandleManager::getInstance()
                                                        .getOrCreateHandle(*data->mPlayer)
                                                        ->draw(data->mPos, data->mDimid);
                        }
                    }

                    titlePacket.sendTo(*pl);
                    subTitlePacket.sendTo(*pl);
                } catch (...) {
                    mSelectors.erase(uuid);
                    continue;
                }
            }
        }
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
    return true;
}
bool LandSelector::uninit() {
    mStabilized.clear();
    auto& bus = ll::event::EventBus::getInstance();
    bus.removeListener(mPlayerUseItemOn);
    return true;
}

LandSelectorData* LandSelector::getSelector(Player& player) {
    auto iter = mSelectors.find(player.getUuid());
    if (iter == mSelectors.end()) {
        return nullptr;
    }
    return iter->second.get();
}
bool LandSelector::isSelectTool(ItemStack const& item) const { return item.getTypeName() == Config::cfg.selector.tool; }
bool LandSelector::isSelecting(Player& player) const {
    auto iter = mSelectors.find(player.getUuid());
    return iter != mSelectors.end() && iter->second->mCanSelect;
}
bool LandSelector::isSelected(Player& player) const {
    auto iter = mSelectors.find(player.getUuid());
    return iter != mSelectors.end() && !iter->second->mCanSelect;
}
bool LandSelector::isSelectedPointA(Player& player) const {
    auto iter = mSelectors.find(player.getUuid());
    return iter != mSelectors.end() && iter->second->mSelectedPointA;
}
bool LandSelector::isSelectedPointB(Player& player) const {
    auto iter = mSelectors.find(player.getUuid());
    return iter != mSelectors.end() && iter->second->mSelectedPointB;
}


bool LandSelector::tryStartSelect(Player& player, int dim, bool draw3D) {
    auto uid = player.getUuid();

    if (mSelectors.find(uid) != mSelectors.end()) {
        return false;
    }

    auto data = std::make_unique<LandSelectorData>(player, dim, draw3D);

    mSelectors[UUIDs(uid)] = std::move(data);
    return true;
}
bool LandSelector::tryCancel(Player& player) {
    auto uid = player.getUuid();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    mSelectors.erase(iter);

    SetTitlePacket title(::SetTitlePacket::TitleType::Clear);
    title.sendTo(player); // clear title

    return true;
}
bool LandSelector::trySelectPointA(Player& player, BlockPos pos) {
    auto uid = player.getUuid();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    iter->second->mSelectedPointA = true;
    iter->second->mPos.mMin_A     = pos;
    return true;
}
bool LandSelector::trySelectPointB(Player& player, BlockPos pos) {
    auto uid = player.getUuid();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    iter->second->mSelectedPointB = true;
    iter->second->mPos.mMax_B     = pos;
    iter->second->mCanSelect      = false;
    iter->second->mCanDraw        = true;
    iter->second->mPos.fix(); // fix  min/max
    return true;
}

bool LandSelector::completeAndRelease(Player& player) {
    auto uid = player.getUuid();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    mSelectors.erase(iter);

    SetTitlePacket title(::SetTitlePacket::TitleType::Clear);
    title.sendTo(player); // clear title

    return true;
}
LandData_sptr LandSelector::makeLandFromSelector(Player& player) {
    auto uid = player.getUuid();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return nullptr;
    }

    auto& data = iter->second;

    return LandData::make(data->mPos, data->mDimid, data->mDraw3D, data->mPlayer->getUuid().asString());
}


// ReSelect
bool LandSelector::isReSelector(Player& player) const {
    auto iter = mSelectors.find(player.getUuid());
    return iter != mSelectors.end() && iter->second->mBindLandData.lock() != nullptr;
}
bool LandSelector::tryReSelect(Player& player, LandData_sptr land) {
    mSelectors.emplace(player.getUuid(), std::make_unique<LandSelectorData>(player, land));
    return true;
}


LandSelectorData::LandSelectorData(Player& player, int dim, bool draw3D)
: mPlayer(&player),
  mDimid(dim),
  mDraw3D(draw3D) {}
LandSelectorData::LandSelectorData(Player& player, LandData_sptr const& landData) {
    this->mPlayer           = &player;
    this->mDimid            = landData->mLandDimid;
    this->mDraw3D           = landData->mIs3DLand;
    this->mBindLandData     = landData;
    this->mIsDrawedOldRange = true;
    this->mOldRangeGeoId =
        DrawHandleManager::getInstance().getOrCreateHandle(player)->draw(landData->mPos, landData->mLandDimid);
}
LandSelectorData::~LandSelectorData() {
#ifdef DEBUG
    printf("LandSelectorData::~LandSelectorData()\n");
#endif
    if (mIsDrawedOldRange) {
        DrawHandleManager::getInstance().getOrCreateHandle(*mPlayer)->remove(mOldRangeGeoId);
    }
    if (mIsDrawedBox) {
#ifdef DEBUG
        printf("LandSelectorData::~LandSelectorData() remove box\n");
#endif
        DrawHandleManager::getInstance().getOrCreateHandle(*mPlayer)->remove(mDrawedBoxGeoId);
    }
}


} // namespace land