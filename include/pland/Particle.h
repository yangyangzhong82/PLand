#pragma once
#include "mc/deps/core/common/bedrock/typeid_t.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include <vector>


namespace land {

class Particle {
public:
    BlockPos mPos1;
    BlockPos mPos2;
    int      mDim;
    bool     mDraw3D;

    bool                  mIsInited{false};
    std::vector<BlockPos> mCalcuatedPos;

    constexpr Particle() = default;
    constexpr Particle(BlockPos pos1, BlockPos pos2, int dim, bool draw3D)
    : mPos1(pos1),
      mPos2(pos2),
      mDim(dim),
      mDraw3D(draw3D) {}

    bool draw(Player& player);
    void fix();
};


} // namespace land
