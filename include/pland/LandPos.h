#pragma once
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/ChunkPos.h"

namespace land {


// 坐标基类
class PosBase {
public:
    int x, y, z;

    std::string toString() const;
};


// 领地坐标
class LandPos {
public:
    PosBase mMin, mMax;

    std::string toString() const;

    std::vector<ChunkPos> getChunks() const;
    std::vector<BlockPos> getBorder() const;
    std::vector<BlockPos> getRange() const;

    bool hasPos(BlockPos const& pos, bool ignoreY = false) const;

    static bool isCollision(LandPos const& pos1, LandPos const& pos2);
};


} // namespace land