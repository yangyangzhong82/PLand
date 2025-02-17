#pragma once
#include "Global.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/ChunkPos.h"


namespace land {


// 坐标基类
class PosBase {
public:
    int x, y, z;

    [[nodiscard]] LDAPI static PosBase make(int x, int y, int z);
    [[nodiscard]] LDAPI static PosBase make(BlockPos const& pos);

    [[nodiscard]] LDAPI std::string toString() const;

    [[nodiscard]] LDAPI bool isZero() const; // xyz是否都为0

    LDAPI PosBase& operator=(PosBase const& pos) = default;
    LDAPI bool     operator==(PosBase const& pos) const;
    LDAPI bool     operator!=(PosBase const& pos) const;
    LDAPI PosBase& operator=(BlockPos const& pos);
    LDAPI bool     operator==(BlockPos const& pos) const;
    LDAPI bool     operator!=(BlockPos const& pos) const;
    LDAPI          operator BlockPos() const;
    LDAPI          operator Vec3() const;
};


// 领地坐标
class LandPos {
public:
    PosBase mMin_A, mMax_B;

    [[nodiscard]] LDAPI static LandPos make(BlockPos const& min, BlockPos const& max);

    LDAPI void fix(); // fix min/max

    [[nodiscard]] LDAPI int getDepth() const;  // (长) X
    [[nodiscard]] LDAPI int getHeight() const; // (高) Y
    [[nodiscard]] LDAPI int getWidth() const;  // (宽) Z
    [[nodiscard]] LDAPI int getSquare() const; // (底面积) X * Z
    [[nodiscard]] LDAPI int getVolume() const; // (总体积) Z * X * Y

    [[nodiscard]] LDAPI std::string toString() const;

    [[nodiscard]] LDAPI std::vector<ChunkPos> getChunks() const;
    [[nodiscard]] LDAPI std::vector<BlockPos> getBorder() const;
    [[nodiscard]] LDAPI std::vector<BlockPos> getRange() const;

    [[nodiscard]] LDAPI bool hasPos(BlockPos const& pos, bool ignoreY = false) const;

    LDAPI bool operator==(LandPos const& pos) const;
    LDAPI bool operator!=(LandPos const& pos) const;

    // 两个领地是否碰撞(重合)
    [[nodiscard]] LDAPI static bool isCollision(LandPos const& pos1, LandPos const& pos2);

    // 两个领地是否满足最小间距
    [[nodiscard]] LDAPI static bool
    isComplisWithMinSpacing(LandPos const& pos1, LandPos const& pos2, bool ignoreY = false);
};


} // namespace land