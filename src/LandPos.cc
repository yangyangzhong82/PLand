#include "pland/LandPos.h"
#include "fmt/format.h"

namespace land {


std::string PosBase::toString() const { return fmt::format("({},{},{})", x, y, z); }


std::string           LandPos::toString() const { return fmt::format("{} => {}", mMin.toString(), mMax.toString()); }
std::vector<ChunkPos> LandPos::getChunks() const {
    std::vector<ChunkPos> chunks;

    int minChunkX = mMin.x >> 4;
    int minChunkZ = mMin.z >> 4;
    int maxChunkX = mMax.x >> 4;
    int maxChunkZ = mMax.z >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            chunks.emplace_back(x, z);
        }
    }
    return chunks;
}

std::vector<BlockPos> LandPos::getBorder() const {
    std::vector<BlockPos> border;
    for (int x = mMin.x; x <= mMax.x; ++x) {
        border.emplace_back(x, mMin.y, mMin.z);
        border.emplace_back(x, mMin.y, mMax.z);
        border.emplace_back(x, mMax.y, mMin.z);
        border.emplace_back(x, mMax.y, mMax.z);
    }
    for (int y = mMin.y + 1; y < mMax.y; ++y) {
        border.emplace_back(mMin.x, y, mMin.z);
        border.emplace_back(mMin.x, y, mMax.z);
        border.emplace_back(mMax.x, y, mMin.z);
        border.emplace_back(mMax.x, y, mMax.z);
    }
    for (int z = mMin.z + 1; z < mMax.z; ++z) {
        border.emplace_back(mMin.x, mMin.y, z);
        border.emplace_back(mMin.x, mMax.y, z);
        border.emplace_back(mMax.x, mMin.y, z);
        border.emplace_back(mMax.x, mMax.y, z);
    }
    return border;
}

std::vector<BlockPos> LandPos::getRange() const {
    std::vector<BlockPos> range;
    for (int x = mMin.x; x <= mMax.x; ++x) {
        range.emplace_back(x, mMax.y, mMin.z);
        range.emplace_back(x, mMax.y, mMax.z);
    }
    for (int z = mMin.z + 1; z < mMax.z; ++z) {
        range.emplace_back(mMin.x, mMax.y, z);
        range.emplace_back(mMax.x, mMax.y, z);
    }
    return range;
}

bool LandPos::hasPos(const BlockPos& pos, bool ignoreY) const {
    if (ignoreY) {
        return pos.x >= mMin.x && pos.x <= mMax.x && pos.z >= mMin.z && pos.z <= mMax.z;
    } else {
        return pos.x >= mMin.x && pos.x <= mMax.x && pos.y >= mMin.y && pos.y <= mMax.y && pos.z >= mMin.z
            && pos.z <= mMax.z;
    }
}

bool LandPos::isCollision(const LandPos& pos1, const LandPos& pos2) {
    return !(
        pos1.mMax.x < pos2.mMin.x || pos1.mMin.x > pos2.mMax.x || pos1.mMax.y < pos2.mMin.y || pos1.mMin.y > pos2.mMax.y
        || pos1.mMax.z < pos2.mMin.z || pos1.mMin.z > pos2.mMax.z
    );
}


} // namespace land