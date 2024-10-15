#include "pland/Particle.h"
#include "mc/deps/core/common/bedrock/typeid_t.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "pland/Config.h"
#include "pland/LandPos.h"


namespace land {

Particle::Particle(BlockPos pos1, BlockPos pos2, int dimid, bool draw3D)
: mPos(LandPos::make(pos1, pos2)),
  mDimid(dimid),
  mDraw3D(draw3D),
  mValid(true) {}
Particle::Particle(LandPos& pos, int dimid, bool draw3D)
: mPos(LandPos(pos)),
  mDimid(dimid),
  mDraw3D(draw3D),
  mValid(true) {}


bool Particle::draw(Player& player, bool refreshCache, bool usePlayerYDraw2D) {
    if (mPackets.empty() || refreshCache) {
        static std::optional<class MolangVariableMap> molangVariables;

        auto pos = mDraw3D ? mPos.getBorder() : mPos.getRange();

        auto dim = VanillaDimensions::toSerializedInt(mDimid);

        mPackets.reserve(pos.size());
        if (mDraw3D) {
            for (auto& p : pos) {
                mPackets.push_back(SpawnParticleEffectPacket(
                    Vec3{p.x + 0.5, p.y + 0.5, p.z + 0.5},
                    Config::cfg.selector.particle,
                    (char)dim,
                    molangVariables
                ));
            }
        } else {
            // 2D
            float y  = usePlayerYDraw2D ? (float)(player.getFeetBlockPos().y + 5) : pos[0].y;
            y       += 0.5; // center
            for (auto& p : pos) {
                mPackets.push_back(SpawnParticleEffectPacket(
                    Vec3{p.x + 0.5, y, p.z + 0.5},
                    Config::cfg.selector.particle,
                    (char)dim,
                    molangVariables
                ));
            }
        }
    }

    for (auto& pkt : mPackets) {
        pkt.sendTo(player);
    }
    return true;
}


} // namespace land