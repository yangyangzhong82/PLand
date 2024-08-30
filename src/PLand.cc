#include "pland/PLand.h"
#include "fmt/core.h"
#include "mc/world/level/BlockPos.h"
#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>


namespace land {

PLand& PLand::getInstance() {
    static PLand instance;
    return instance;
}

bool PLand::init() {
    // TODO: load land data from file
    return true;
}
bool PLand::save() {
    // TODO: save land data to file
    return true;
}

bool PLand::isOperator(UUIDs const& uuid) const {
    return std::find(mLandOperators.begin(), mLandOperators.end(), uuid) != mLandOperators.end();
}
bool PLand::addOperator(UUIDs const& uuid) {
    if (isOperator(uuid)) {
        return false;
    }
    mLandOperators.push_back(uuid);
    return true;
}
bool PLand::removeOperator(UUIDs const& uuid) {
    auto iter = std::find(mLandOperators.begin(), mLandOperators.end(), uuid);
    if (iter == mLandOperators.end()) {
        return false;
    }
    mLandOperators.erase(iter);
    return true;
}

bool PLand::hasLand(LandID id) const { return mLandCache.find(id) != mLandCache.end(); }
bool PLand::addLand(LandDataPtr land) {
    if (land == nullptr) {
        return false;
    }
    if (land->mLandID != uint64_t(-1)) {
        return false; // land already added
    }
    land->mLandID = generateLandID();
    mLandCache.emplace(land->mLandID, land);

    auto chs = land->mPos.getChunks();
    for (auto& c : chs) {
        auto& ls   = mLandMap[land->mLandDim][getChunkID(c.x, c.z)];
        auto  iter = std::find(ls.begin(), ls.end(), land->mLandID);
        if (iter == ls.end()) {
            ls.push_back(land->mLandID);
        } else {
            return false; // land already added
        }
    }

    return true;
}
bool PLand::removeLand(LandID landId) {
    auto landIt = mLandCache.find(landId);
    if (landIt == mLandCache.end()) {
        return false;
    }
    auto land = landIt->second;

    for (auto& c : land->mPos.getChunks()) {
        auto& ls   = mLandMap[land->mLandDim][getChunkID(c.x, c.z)];
        auto  iter = std::find(ls.begin(), ls.end(), land->mLandID);
        if (iter != ls.end()) {
            ls.erase(iter);
        }
    }
    mLandCache.erase(landIt);
    return true;
}

LandDataPtr PLand::getLand(LandID id) const {
    auto landIt = mLandCache.find(id);
    if (landIt != mLandCache.end()) {
        return landIt->second;
    }
    return nullptr;
}

LandPermType PLand::getPermType(UUIDs const& uuid, LandID id, bool ignoreOperator) const {
    if (!ignoreOperator && isOperator(uuid)) return LandPermType::Operator;

    auto land = getLand(id);
    if (land) {
        return land->getPermType(uuid);
    }

    return LandPermType::Guest;
}


LandID PLand::generateLandID() { return static_cast<LandID>(mLandCache.size()) + 1; }

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


// static function
ChunkID PLand::getChunkID(int x, int z) {
    uint64_t ux       = static_cast<uint64_t>(std::abs(x));
    uint64_t uz       = static_cast<uint64_t>(std::abs(z));
    uint64_t signBits = 0;
    if (x >= 0) signBits |= (1ULL << 63);
    if (z >= 0) signBits |= (1ULL << 62);
    return signBits | (ux << 31) | (uz & 0x7FFFFFFF);
}
std::pair<int, int> PLand::parseChunkID(ChunkID id) {
    bool xPositive = (id & (1ULL << 63)) != 0;
    bool zPositive = (id & (1ULL << 62)) != 0;
    int  x         = static_cast<int>((id >> 31) & 0x7FFFFFFF);
    int  z         = static_cast<int>(id & 0x7FFFFFFF);
    if (!xPositive) x = -x;
    if (!zPositive) z = -z;
    return {x, z};
}


} // namespace land