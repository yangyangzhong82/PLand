#include "pland/PLand.h"
#include "fmt/core.h"
#include "ll/api/data/KeyValueDB.h"
#include "mc/world/level/BlockPos.h"
#include "mod/MyMod.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/utils/JSON.h"
#include "pland/utils/Utils.h"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>


namespace land {

namespace fs = std::filesystem;
#define DB_DIR_NAME      "db"
#define DB_KEY_OPERATORS "operators"

bool PLand::init() {
    auto dir = my_mod::MyMod::getInstance().getSelf().getDataDir() / DB_DIR_NAME;
    if (!fs::exists(dir)) {
        fs::create_directory(dir);
    }

    if (!mDB) {
        mDB = std::make_unique<ll::data::KeyValueDB>(dir);
    }


    // init and load operators
    if (!mDB->has(DB_KEY_OPERATORS)) {
        mDB->set(DB_KEY_OPERATORS, "[]"); // empty array
    }
    auto ops = JSON::parse(*mDB->get(DB_KEY_OPERATORS));
    JSON::jsonToStructNoMerge(ops, mLandOperators);


    // load land data
    mDB->iter([this](auto key, auto value) {
        if (key == DB_KEY_OPERATORS) return true; // skip operators
        auto json = JSON::parse(value);
        auto land = LandData::make();

        JSON::jsonToStruct(json, *land);

        mLandCache.emplace(land->getLandID(), land);
        return true;
    });


    auto& logger = my_mod::MyMod::getInstance().getSelf().getLogger();
    logger.info("已加载 {} 位操作员", mLandOperators.size());
    logger.info("已加载 {} 块领地数据", mLandCache.size());
    return _initCache();
}
bool PLand::save() {
    mDB->set(DB_KEY_OPERATORS, JSON::structToJsonString(mLandOperators));

    for (auto& [id, land] : mLandCache) {
        mDB->set(std::to_string(land->mLandID), JSON::structToJsonString(*land));
    }

    return true;
}
bool PLand::_initCache() {
    for (auto& [id, land] : mLandCache) {
        auto& chunkMap = mLandMap[LandDimid(land->getLandDimid())]; // 区块表

        auto chs = land->mPos.getChunks();
        for (auto& ch : chs) {
            auto  chunkID      = PLand::getChunkID(ch.x, ch.z);
            auto& chunkLandVec = chunkMap[chunkID]; // 区块领地数组

            if (!some(chunkLandVec, land->getLandID())) {
                chunkLandVec.push_back(land->getLandID());
            }
        }
    }
    my_mod::MyMod::getInstance().getSelf().getLogger().info("初始化领地缓存系统完成");
    return true;
}


PLand& PLand::getInstance() {
    static PLand instance;
    return instance;
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
        auto& ls   = mLandMap[land->mLandDimid][getChunkID(c.x, c.z)];
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
        auto& ls   = mLandMap[land->mLandDimid][getChunkID(c.x, c.z)];
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