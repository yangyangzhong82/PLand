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
    bool    mValid{false};

    std::vector<SpawnParticleEffectPacket> mPackets;

    LDAPI Particle() = default;
    LDAPI Particle(LandPos& pos, int dimid, bool draw3D);
    LDAPI Particle(BlockPos pos1, BlockPos pos2, int dimid, bool draw3D);

    LDAPI bool draw(Player& player, bool refreshCache = false, bool usePlayerYDraw2D = true);
};


} // namespace land
