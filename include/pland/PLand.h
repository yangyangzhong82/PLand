#pragma once
#include "ll/api/data/KeyValueDB.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>


namespace land {


class PLand {
public:
    PLand()                        = default;
    PLand(const PLand&)            = delete;
    PLand& operator=(const PLand&) = delete;

    std::unique_ptr<ll::data::KeyValueDB> mDB;

    //                 维度                         区块            领地
    std::unordered_map<LandDim, std::unordered_map<ChunkID, std::vector<LandID>>> mLandMap;   // 领地映射表
    std::unordered_map<LandID, LandDataPtr>                                       mLandCache; // 领地缓存

    std::vector<UUIDs> mLandOperators; // 领地操作员

    static PLand& getInstance();

    bool init();
    bool save();

    bool isOperator(UUIDs const& uuid) const;
    bool addOperator(UUIDs const& uuid);
    bool removeOperator(UUIDs const& uuid);

    bool addLand(LandDataPtr land);

    bool removeLand(LandID id);

    bool hasLand(LandID id) const;

    LandDataPtr getLand(LandID id) const;

    LandPermType getPermType(UUIDs const& uuid, LandID id = 0, bool ignoreOperator = false) const;

    LandDataPtr              getLandAt(BlockPos const& pos) const;
    std::vector<LandDataPtr> getLandAt(BlockPos const& center, int radius) const;         // 半径内的领地
    std::vector<LandDataPtr> getLandAt(BlockPos const& pos1, BlockPos const& pos2) const; // 矩形内的领地

    LandID generateLandID();

    static ChunkID             getChunkID(int x, int z);
    static std::pair<int, int> parseChunkID(ChunkID id);
};


} // namespace land