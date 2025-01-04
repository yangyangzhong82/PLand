#include "pland/SafeTeleport.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/block/Block.h"
#include "mod/MyMod.h"
#include "pland/Config.h"
#include "pland/Global.h"
#include "pland/utils/MC.h"
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>



namespace land {


struct TeleportData {
    Vec3     mTargetPos;   // 目标位置
    int      mTargetDimid; // 目标维度
    Player*  mPlayer;      // 玩家
    uint64_t mID;          // 唯一ID
    Vec3     mSourcePos;   // 原始位置

    short mScheduleCounter = 0; // 调度计数器

    static constexpr short SCHEDULE_COUNTER_MAX = 80;
};


class SafeTeleport::SafeTeleportImpl {
public:
    SafeTeleportImpl()                                   = default;
    SafeTeleportImpl(const SafeTeleportImpl&)            = delete;
    SafeTeleportImpl& operator=(const SafeTeleportImpl&) = delete;

    std::unordered_map<uint64_t, TeleportData> mTeleportQueue;
    uint64_t                                   mIDCounter = 0; // 唯一ID计数器

    uint64_t insert(Player& player, Vec3 const& pos, int dimid, Vec3 const& sourcePos) {
        uint64_t id        = ++mIDCounter;
        mTeleportQueue[id] = {Vec3(pos), dimid, &player, id, Vec3(sourcePos)};
        return id;
    }
    bool remove(uint64_t id) {
        mTeleportQueue.erase(id);
        return true;
    }

    void findSafePos(Player& player, Vec3 const& targetPos, int dimid, Vec3 const& sourcePos) {
        static const std::vector<string> dangerousBlocks = {"minecraft:lava", "minecraft:flowing_lava"};

        // auto& logger = my_mod::MyMod::getInstance().getSelf().getLogger();

        auto& dim = player.getDimension();
        auto& bs  = player.getDimensionBlockSource();

        short const start = dim.getHeight();    // 世界高度
        short const end   = dim.getMinHeight(); // 地面高度

        short current    = start; // 当前高度
        short safeHeight = -1;    // 安全高度

        Block*   headBlock = nullptr; // 头块
        Block*   legBlock  = nullptr; // 脚块
        BlockPos currentPos(targetPos.x, current, targetPos.z);
        while (current > end) {
            currentPos.y = current;
            Block& bl    = const_cast<Block&>(bs.getBlock(currentPos));

            // logger.debug(
            //     "current: {}, currentPos.y: {}, bl: {}, headBlock: {}, legBlock: {}",
            //     current,
            //     currentPos.y,
            //     bl.getTypeName(),
            //     headBlock ? headBlock->getTypeName() : "nullptr",
            //     legBlock ? legBlock->getTypeName() : "nullptr"
            // );

            if (std::find(dangerousBlocks.begin(), dangerousBlocks.end(), bl.getTypeName()) == dangerousBlocks.end()
                && !bl.isAir() && headBlock->isAir() && legBlock->isAir()) {
                safeHeight = current;
                break;
            }

            if (!headBlock && !legBlock) {
                headBlock = &bl;
                legBlock  = &bl;
            }

            // 交换头块和脚块
            headBlock = legBlock;
            legBlock  = &bl;
            current--;
        }

        if (safeHeight == -1) {
            mc::sendText<mc::LogLevel::Info>(player, "无法找到安全位置"_tr());
            player.teleport(sourcePos, dimid); //  返回原位置
            return;
        }

        player.teleport(Vec3(currentPos.x + 0.5, safeHeight + 1, currentPos.z + 0.5), dimid);
    }
    void findSafePos(TeleportData& data) {
        findSafePos(*data.mPlayer, data.mTargetPos, data.mTargetDimid, data.mSourcePos);
    }


    void execute(uint64_t id) {
        // GlobalTickScheduler.add<ll::schedule::DelayTask>(10_tick, [id, this]() {
        //     auto iter = mTeleportQueue.find(id);
        //     if (iter == mTeleportQueue.end()) {
        //         return;
        //     }

        //     auto& dt = iter->second;
        //     dt.mPlayer->teleport(Vec3(dt.mTargetPos.x, 666, dt.mTargetPos.z), dt.mTargetDimid); // 临时传送

        //     auto& bs = dt.mPlayer->getDimensionBlockSource();
        //     if (bs.isChunkFullyLoaded(ChunkPos(dt.mTargetPos), bs.getChunkSource())) {
        //         findSafePos(dt);
        //         remove(id);
        //     } else {
        //         if (++dt.mScheduleCounter > TeleportData::SCHEDULE_COUNTER_MAX) {
        //             mc::sendText<mc::LogLevel::Info>(*dt.mPlayer, "无法找到安全位置, 区块加载超时"_tr());
        //             dt.mPlayer->teleport(dt.mSourcePos, dt.mTargetDimid); //  返回原位置
        //             remove(id);
        //             return;
        //         }
        //         execute(id);
        //     }
        // });
    }


    void tryTeleport(Player& player, Vec3 const& pos, int dimid) {
        auto& bs        = player.getDimensionBlockSource();
        Vec3  sourcePos = player.getPosition();

        if (bs.isChunkFullyLoaded(ChunkPos(pos), bs.getChunkSource())) {
            findSafePos(player, pos, dimid, sourcePos);
            return;
        }

        execute(insert(player, pos, dimid, sourcePos));
    }


    static SafeTeleportImpl& getInstance() {
        static SafeTeleportImpl instance;
        return instance;
    }
};


void SafeTeleport::teleportTo(Player& player, Vec3 const& pos, int dimid) {
    if (!Config::cfg.land.landTp) {
        mc::sendText<mc::LogLevel::Info>(player, "此功能已被管理员关闭"_tr());
        return;
    }

    SafeTeleportImpl::getInstance().tryTeleport(player, pos, dimid);
}
SafeTeleport& SafeTeleport::getInstance() {
    static SafeTeleport instance;
    return instance;
}


} // namespace land
