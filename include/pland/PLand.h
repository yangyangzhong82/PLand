#pragma once
#include "ll/api/data/KeyValueDB.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandPos.h"
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
    std::unordered_map<LandDimid, std::unordered_map<ChunkID, std::vector<LandID>>> mLandMap;   // 领地映射表
    std::unordered_map<LandID, LandDataPtr>                                         mLandCache; // 领地缓存

    std::vector<UUIDs> mLandOperators; // 领地操作员

    static PLand& getInstance();

    bool init();
    bool save();
    bool _initCache(); // private

    bool mThreadCanRun{true}; // private
    void _startThread();      // private
    void _stopThread();       // private

    bool isOperator(UUIDs const& uuid) const;
    bool addOperator(UUIDs const& uuid);
    bool removeOperator(UUIDs const& uuid);

    bool hasLand(LandID id) const;
    bool addLand(LandDataPtr land);
    bool removeLand(LandID id);

    bool refreshLandRange(LandDataPtr ptr); // 刷新领地范围

    LandDataPtr              getLand(LandID id) const;                           // 获取领地数据
    std::vector<LandDataPtr> getLands();                                         // 获取所有领地数据
    std::vector<LandDataPtr> getLands(LandDimid dimid);                          // 获取维度领地数据
    std::vector<LandDataPtr> getLands(UUIDs const& uuid);                        // 获取玩家领地数据
    std::vector<LandDataPtr> getLands(UUIDs const& uuid, LandDimid dimid) const; // 获取玩家维度领地数据

    LandPermType getPermType(UUIDs const& uuid, LandID id = 0, bool ignoreOperator = false) const; // 获取领地权限类型

    // 获取领地数据
    LandDataPtr getLandAt(BlockPos const& pos, LandDimid dimid) const;
    // 半径内的领地
    std::vector<LandDataPtr> getLandAt(BlockPos const& center, int radius, LandDimid dimid) const;
    // 矩形内的领地
    std::vector<LandDataPtr> getLandAt(BlockPos const& pos1, BlockPos const& pos2, LandDimid dimid) const;

    LandID generateLandID();

    static ChunkID             getChunkID(int x, int z);
    static std::pair<int, int> parseChunkID(ChunkID id);
};


} // namespace land