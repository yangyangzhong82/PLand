#include "pland/Particle.h"
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/schedule/Scheduler.h"
#include "ll/api/schedule/Task.h"
#include "mc/deps/core/common/bedrock/typeid_t.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/registry/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/Spawner.h"
#include "mc/world/level/block/Block.h"
#include "mod/MyMod.h"
#include "pland/Global.h"
#include "pland/LandPos.h"
#include "pland/utils/Date.h"
#include "pland/utils/MC.h"
#include <optional>
#include <unordered_map>
#include <utility>


namespace land {


void Particle::fix() {
    if (mPos1.x > mPos2.x) std::swap(mPos1.x, mPos2.x);
    if (mPos1.y > mPos2.y) std::swap(mPos1.y, mPos2.y);
    if (mPos1.z > mPos2.z) std::swap(mPos1.z, mPos2.z);
}
bool Particle::draw(Player& player) {
    std::optional<class MolangVariableMap> molang;

    if (mCalcuatedPos.empty()) {
        auto data     = LandPos::make(mPos1, mPos2);
        mCalcuatedPos = mDraw3D ? data->getBorder() : data->getRange();
    }

    auto& level = player.getLevel();
    auto& dim   = player.getDimension();
    for (auto& pos : mCalcuatedPos) {
        level.spawnParticleEffect(mParticle, pos, &dim);
    }

    return true;
}


LandSelector& LandSelector::getInstance() {
    static LandSelector instance;
    return instance;
}

ll::event::ListenerPtr                 mPlayerUseItemOn;
ll::schedule::GameTickScheduler        mTickScheduler;
std::unordered_map<UUIDm, std::time_t> mStabilized; // 防抖
bool                                   LandSelector::init() {
    mPlayerUseItemOn = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerInteractBlockEvent>(
        [this](ll::event::PlayerInteractBlockEvent const& ev) {
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

            if (!this->isSelectedPoint1(pl)) {
                if (this->trySelectPoint1(pl, ev.clickPos())) {
                    mc::sendText(pl, "已选择点1 \"{}\"", ev.clickPos().toString());
                    mStabilized[pl.getUuid()] = Date::future(40 / 1000).getTime(); // 40ms
                } else {
                    mc::sendText<mc::LogLevel::Error>(pl, "选择点1失败");
                }

            } else if (!this->isSelectedPoint2(pl) && this->isSelectedPoint1(pl)) {
                if (this->trySelectPoint2(pl, ev.clickPos())) {
                    mc::sendText(pl, "已选择点2 \"{}\"", ev.clickPos().toString());
                } else {
                    mc::sendText<mc::LogLevel::Error>(pl, "选择点2失败");
                }
            }
        }
    );

    using ll::chrono_literals::operator""_tick;
    mTickScheduler.add<ll::schedule::RepeatTask>(10_tick, [this]() {
        for (auto& [uuid, data] : mSelectors) {
            if (!data.mPlayer) {
                mSelectors.erase(uuid); // 玩家断开连接
                continue;
            }

            if (data.mCanDraw) {
                if (!data.mParticle.mIsInited) {
                    data.mParticle = Particle(data.mPos1, data.mPos2, data.mDim, data.mDraw3D);
                    data.mParticle.fix();
                }
                data.mParticle.draw(*data.mPlayer);
            }
        }
    });
    return true;
}
bool LandSelector::uninit() { return ll::event::EventBus::getInstance().removeListener(mPlayerUseItemOn); }

std::optional<LandSelectorData> LandSelector::getSelector(Player& player) const {
    auto iter = mSelectors.find(player.getUuid().asString());
    if (iter == mSelectors.end()) {
        return std::nullopt;
    }

    return iter->second;
}
bool LandSelector::isSelectTool(ItemStack const& item) const { return item.getTypeName() == this->mTools; }
bool LandSelector::isSelecting(Player& player) const {
    auto iter = mSelectors.find(player.getUuid().asString());
    return iter != mSelectors.end() && iter->second.mCanSelect;
}
bool LandSelector::isSelected(Player& player) const {
    auto iter = mSelectors.find(player.getUuid().asString());
    return iter != mSelectors.end() && !iter->second.mCanSelect;
}
bool LandSelector::isSelectedPoint1(Player& player) const {
    auto iter = mSelectors.find(player.getUuid().asString());
    return iter != mSelectors.end() && iter->second.mSelectedPoint1;
}
bool LandSelector::isSelectedPoint2(Player& player) const {
    auto iter = mSelectors.find(player.getUuid().asString());
    return iter != mSelectors.end() && iter->second.mSelectedPoint2;
}


bool LandSelector::tryStartSelect(Player& player, int dim, bool draw3D) {
    auto uid = player.getUuid().asString();

    if (mSelectors.find(uid) != mSelectors.end()) {
        return false;
    }

    mSelectors[UUIDs(uid)] = LandSelectorData(player, dim, draw3D);
    return true;
}
bool LandSelector::tryStopSelect(Player& player) {
    auto uid = player.getUuid().asString();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    mSelectors.erase(iter);
    return true;
}
bool LandSelector::trySelectPoint1(Player& player, BlockPos pos) {
    auto uid = player.getUuid().asString();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    iter->second.mSelectedPoint1 = true;
    iter->second.mPos1           = pos;
    return true;
}
bool LandSelector::trySelectPoint2(Player& player, BlockPos pos) {
    auto uid = player.getUuid().asString();

    auto iter = mSelectors.find(uid);
    if (iter == mSelectors.end()) {
        return false;
    }

    iter->second.mSelectedPoint2 = true;
    iter->second.mPos2           = pos;
    iter->second.mCanSelect      = false;
    iter->second.mCanDraw        = true;
    return true;
}

} // namespace land