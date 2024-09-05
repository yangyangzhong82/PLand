#pragma once
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/ChunkPos.h"
#include <memory>

namespace land {


// 坐标基类
class PosBase {
public:
    int x, y, z;

    static PosBase make(int x, int y, int z);
    static PosBase make(BlockPos const& pos);

    std::string toString() const;
    BlockPos    toBlockPos() const;

    PosBase& operator=(PosBase const& pos) = default;
    bool     operator==(PosBase const& pos) const;
    bool     operator!=(PosBase const& pos) const;
    PosBase& operator=(BlockPos const& pos);
    bool     operator==(BlockPos const& pos) const;
    bool     operator!=(BlockPos const& pos) const;
};


// 领地坐标
class LandPos {
public:
    PosBase mMin_A, mMax_B;

    static LandPos make(BlockPos const& min, BlockPos const& max);

    void fix();

    std::string toString() const;

    std::vector<ChunkPos> getChunks() const;
    std::vector<BlockPos> getBorder() const;
    std::vector<BlockPos> getRange() const;

    bool hasPos(BlockPos const& pos, bool ignoreY = false) const;

    static bool isCollision(LandPos const& pos1, LandPos const& pos2);
};


} // namespace land