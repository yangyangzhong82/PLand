#include "pland/LandSelector.h"
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/event/player/PlayerLeaveEvent.h"
#include "ll/api/schedule/Scheduler.h"
#include "ll/api/schedule/Task.h"
#include "mc/world/item/registry/ItemStack.h"
#include "mod/MyMod.h"
#include "pland/Config.h"
#include "pland/GUI.h"
#include "pland/Global.h"
#include "pland/utils/Date.h"
#include "pland/utils/MC.h"
#include <optional>
#include <unordered_map>

#include "mc/network/packet/SetTitlePacket.h"

namespace land {


LandSelector& LandSelector::getInstance() {
    static LandSelector instance;
    return instance;
}

ll::event::ListenerPtr                 mPlayerUseItemOn;
ll::event::ListenerPtr                 mPlayerLeave;
ll::schedule::GameTickScheduler        mTickScheduler;
std::unordered_map<UUIDm, std::time_t> mStabilized; // 防抖
bool                                   LandSelector::init() {
    auto& bus = ll::event::EventBus::getInstance();
    mPlayerUseItemOn =
        bus.emplaceListener<ll::event::PlayerInteractBlockEvent>([this](ll::event::PlayerInteractBlockEvent const& ev) {
            auto& pl = ev.self();

            if (!this->isSelectTool(ev.item()) || !this->isSelecting(pl) || this->isSelected(pl)) {
                return;
            }

            if (auto iter = mStabilized.find(pl.getUuid()); iter != mStabilized.end()) {
                if (iter->second >= Date::now().getTime()) {
                    return;
                }
            }

#ifdef DEBUG
            my_mod::MyMod::getInstance().getSelf().getLogger().info(
                "PlayerInteractBlockEvent: {}",
                ev.clickPos().toString()
            );
#endif

            if (!this->isSelectedPointA(pl)) {
                if (this->trySelectPointA(pl, ev.clickPos())) {
                    mc::sendText(pl, "已选择点A \"{}\""_tr(ev.clickPos().toString()));
                    mStabilized[pl.getUuid()] = Date::future(50 / 1000).getTime(); // 50ms
                } else {
                    mc::sendText<mc::LogLevel::Error>(pl, "选择失败"_tr());
                }

            } else if (!this->isSelectedPointB(pl) && this->isSelectedPointA(pl)) {
                if (this->trySelectPointB(pl, ev.clickPos())) {
                    mc::sendText(pl, "已选择点B \"{}\""_tr(ev.clickPos().toString()));
                    SelectorChangeYGui::send(pl); // 发送GUI
                } else {
                    mc::sendText<mc::LogLevel::Error>(pl, "选择失败"_tr());
                }
            }
        });

    mPlayerLeave = bus.emplaceListener<ll::event::PlayerLeaveEvent>([this](ll::event::PlayerLeaveEvent const& ev) {
        auto iter = this->mSelectors.find(ev.self().getUuid().asString());
        if (iter != this->mSelectors.end()) {
            this->mSelectors.erase(iter);
        }
    });

    using ll::chrono_literals::operator""_tick; // 1s = 20_tick
    mTickScheduler.add<ll::schedule::RepeatTask>(20_tick, [this]() {
        for (auto& [uuid, data] : mSelectors) {
            try {
                auto pl = data.mPlayer; // 玩家指针
                if (pl == nullptr) {
                    mSelectors.erase(uuid); // 玩家断开连接
                    continue;
                }

                SetTitlePacket titlePacket(SetTitlePacket::TitleType::Title);
                SetTitlePacket subTitlePacket(SetTitlePacket::TitleType::Subtitle);
                if (!data.mSelectedPointA) {
                    titlePacket.mTitleText = "[ 选区器 ]"_tr();
                    subTitlePacket.mTitleText = "使用 /pland set a 或使用 {} 选择点A"_tr(Config::cfg.selector.tool);

                } else if (!data.mSelectedPointB) {
                    titlePacket.mTitleText = "[ 选区器 ]"_tr();
                    subTitlePacket.mTitleText = "使用 /pland set b 或使用 {} 选择点B"_tr(Config::cfg.selector.tool);

                } else if (data.mCanDraw && Config::cfg.selector.drawParticle) {
                    titlePacket.mTitleText    = "[ 选区完成 ]"_tr();
                    subTitlePacket.mTitleText = "使用 /pland buy 呼出购买菜单"_tr(Config::cfg.selector.tool);

                    // 绘制粒子
                    if (!data.mIsInitedParticle) {
                        data.mParticle.mPos    = data.mPos;
                        data.mIsInitedParticle = true;
                    }
                    data.mParticle.draw(*pl);
                }

                titlePacket.sendTo(*pl);
                subTitlePacket.sendTo(*pl);
            } catch (...) {
                mSelectors.erase(uuid);
                continue;
            }
        }
    });
    return true;
}
bool LandSelector::uninit() {
    mTickScheduler.clear();
    mStabilized.clear();
    ll::event::EventBus::getInstance().removeListener(mPlayerUseItemOn);
    return true;
}

LandSelectorData* LandSelector::getSelector(Player& player) {
    auto iter = mSelectors.find(player.getUuid().asString());
    if (iter == mSelectors.end()) {
        return nullptr;
    }

    return &iter->second;
}
bool LandSelector::isSelectTool(ItemStack const& item) const { return item.getTypeName() == Config::cfg.selector.tool; }
bool LandSelector::isSelecting(Player& player) const {
    auto iter = mSelectors.find(player.getUuid().asString());
    return iter != mSelectors.end() && iter->second.mCanSelect;
}
bool LandSelector::isSelected(Player& player) const {
    auto iter = mSelectors.find(player.getUuid().asString());
    return iter != mSelectors.end() && !iter->second.mCanSelect;
}
bool LandSelector::isSelectedPointA(Player& player) const {
    auto iter = mSelectors.find(player.getUuid().asString());
    return iter != mSelectors.end() && iter->second.mSelectedPointA;
}
bool LandSelector::isSelectedPointB(Player& player) const {
    auto iter = mSelectors.find(player.getUuid().asString());
    return iter != mSelectors.end() && iter->second.mSelectedPointB;
}


bool LandSelector::tryStartSelect(Player& player, int dim, bool draw3D) {
    auto uid = player.getUuid().asString();

    if (mSelectors.find(uid) != mSelectors.end()) {
        return false;
    }

    mSelectors[UUIDs(uid)] = LandSelectorData(player, dim, draw3D);
    return true;
}
bool LandSelector::tryCancelSelect(Player& player) {
    auto uid = player.getUuid().asString();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    mSelectors.erase(iter);
    return true;
}
bool LandSelector::trySelectPointA(Player& player, BlockPos pos) {
    auto uid = player.getUuid().asString();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    iter->second.mSelectedPointA = true;
    iter->second.mPos.mMin_A     = pos;
    return true;
}
bool LandSelector::trySelectPointB(Player& player, BlockPos pos) {
    auto uid = player.getUuid().asString();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    iter->second.mSelectedPointB = true;
    iter->second.mPos.mMax_B     = pos;
    iter->second.mCanSelect      = false;
    iter->second.mCanDraw        = true;
    iter->second.mPos.fix(); // fix  min/max
    return true;
}

} // namespace land