#include "pland/Particle.h"
#include "mc/deps/core/common/bedrock/typeid_t.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "pland/Config.h"
#include "pland/LandPos.h"
#include "pland/utils/MC.h"
#include <utility>

#include "mc/world/level/dimension/VanillaDimensions.h"

namespace land {


void Particle::fix() {
    if (mPos1.x > mPos2.x) std::swap(mPos1.x, mPos2.x);
    if (mPos1.y > mPos2.y) std::swap(mPos1.y, mPos2.y);
    if (mPos1.z > mPos2.z) std::swap(mPos1.z, mPos2.z);
}
bool Particle::draw(Player& player) {
    if (mPackets.empty()) {
        static std::optional<class MolangVariableMap> molangVariables;

        auto data = LandPos::make(mPos1, mPos2);
        auto pos  = mDraw3D ? data->getBorder() : data->getRange();

        auto dim = VanillaDimensions::toSerializedInt(mDim);

        mPackets.reserve(pos.size());
        for (auto& p : pos) {
            mPackets.push_back(SpawnParticleEffectPacket(
                Vec3{p.x + 0.5, p.y + 0.5, p.z + 0.5},
                Config::cfg.selector.particle,
                (char)dim,
                molangVariables
            ));
        }
    }

    for (auto& pkt : mPackets) {
        pkt.sendTo(player);
    }
    return true;
}


} // namespace land