#pragma once
#include "mc/deps/core/common/bedrock/typeid_t.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "pland/LandPos.h"
#include <vector>


namespace land {

class Particle {
public:
    LandPos mPos;
    int     mDimid;
    bool    mDraw3D;

    std::vector<SpawnParticleEffectPacket> mPackets;

    Particle() = default;
    Particle(LandPos& pos, int dimid, bool draw3D);
    Particle(BlockPos pos1, BlockPos pos2, int dimid, bool draw3D);

    bool draw(Player& player, bool refreshCache = false, bool usePlayerYDraw2D = true);
};


} // namespace land
