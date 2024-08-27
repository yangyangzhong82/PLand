#include "pland/PLand.h"
#include "fmt/core.h"
#include "mc/world/level/BlockPos.h"
#include <utility>
#include <vector>


namespace land {


LandDataPtr PLand::getLandAt(BlockPos const& pos) const {
    ChunkID chunkId = getChunkID(pos.x >> 4, pos.z >> 4);
    auto    dimIt   = mLandMap.find(pos.y);
    if (dimIt != mLandMap.end()) {
        auto chunkIt = dimIt->second.find(chunkId);
        if (chunkIt != dimIt->second.end()) {
            for (const auto& landId : chunkIt->second) {
                auto landIt = mLandCache.find(landId);
                if (landIt != mLandCache.end()
                    && landIt->second->getLandPos().hasPos(pos, !landIt->second->is3DLand())) {
                    return landIt->second;
                }
            }
        }
    }
    return nullptr;
}

std::vector<LandDataPtr> PLand::getLandAt(BlockPos const& center, int radius) const {
    std::vector<LandDataPtr> lands;

    int minChunkX = (center.x - radius) >> 4;
    int minChunkZ = (center.z - radius) >> 4;
    int maxChunkX = (center.x + radius) >> 4;
    int maxChunkZ = (center.z + radius) >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            ChunkID chunkId = getChunkID(x, z);
            auto    dimIt   = mLandMap.find(center.y);
            if (dimIt != mLandMap.end()) {
                auto chunkIt = dimIt->second.find(chunkId);
                if (chunkIt != dimIt->second.end()) {
                    for (const auto& landId : chunkIt->second) {
                        auto landIt = mLandCache.find(landId);
                        if (landIt != mLandCache.end() && landIt->second->isRadiusInLand(center, radius)) {
                            lands.push_back(landIt->second);
                        }
                    }
                }
            }
        }
    }
    return lands;
}

std::vector<LandDataPtr> PLand::getLandAt(BlockPos const& pos1, BlockPos const& pos2) const {
    std::vector<LandDataPtr> lands;

    int minChunkX = std::min(pos1.x, pos2.x) >> 4;
    int minChunkZ = std::min(pos1.z, pos2.z) >> 4;
    int maxChunkX = std::max(pos1.x, pos2.x) >> 4;
    int maxChunkZ = std::max(pos1.z, pos2.z) >> 4;

    for (int x = minChunkX; x <= maxChunkX; ++x) {
        for (int z = minChunkZ; z <= maxChunkZ; ++z) {
            ChunkID chunkId = getChunkID(x, z);
            auto    dimIt   = mLandMap.find(pos1.y);
            if (dimIt != mLandMap.end()) {
                auto chunkIt = dimIt->second.find(chunkId);
                if (chunkIt != dimIt->second.end()) {
                    for (const auto& landId : chunkIt->second) {
                        auto landIt = mLandCache.find(landId);
                        if (landIt != mLandCache.end() && landIt->second->isAABBInLand(pos1, pos2)) {
                            lands.push_back(landIt->second);
                        }
                    }
                }
            }
        }
    }
    return lands;
}


ChunkID PLand::getChunkID(int x, int z) {
    return (static_cast<uint64_t>(x) << 32) | (static_cast<uint64_t>(z) & 0xFFFFFFFF);
}
std::pair<int, int> PLand::parseChunkID(ChunkID id) {
    int x = static_cast<int>(id >> 32);
    int z = static_cast<int>(id & 0xFFFFFFFF);
    return {x, z};
}


} // namespace land