#include "pland/Particle.h"
#include "mc/deps/core/common/bedrock/typeid_t.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "pland/Config.h"
#include "pland/LandPos.h"
#include "pland/utils/MC.h"
#include <utility>


namespace land {


void Particle::fix() {
    if (mPos1.x > mPos2.x) std::swap(mPos1.x, mPos2.x);
    if (mPos1.y > mPos2.y) std::swap(mPos1.y, mPos2.y);
    if (mPos1.z > mPos2.z) std::swap(mPos1.z, mPos2.z);
}
bool Particle::draw(Player& player) {
    if (mCalcuatedPos.empty()) {
        auto data     = LandPos::make(mPos1, mPos2);
        mCalcuatedPos = mDraw3D ? data->getBorder() : data->getRange();
    }

    auto& level = player.getLevel();
    auto  dim   = level.getDimension(mDim);
    if (!dim) {
        mc::sendText<mc::LogLevel::Error>(player, "Plugin Error: Failed to get dimension at {}"_tr(mDim));
        return false;
    }
    for (auto& pos : mCalcuatedPos) {
        level.spawnParticleEffect(Config::cfg.selector.particle, pos, dim.get());
    }

    return true;
}


} // namespace land