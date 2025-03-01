#include "pland/SafeTeleport.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerDisconnectEvent.h"
#include "ll/api/thread/ServerThreadExecutor.h"
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
#include <concurrentqueue.h>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <utility>
#include <vector>


namespace land {


using DimensionPos = std::pair<Vec3, int>;

enum class TaskState {
    WaitingProcess,              // 等待处理
    WaitingChunkLoad,            // 等待区块加载
    ChunkLoadedWaitingProcess,   // 区块加载完成，等待处理
    FindingSafePos,              // 找安全位置
    FindedSafePosWaitingProcess, // 找到安全位置，等待处理
    TaskCompleted,               // 任务完成
    WaitChunkLoadTimeout,        // 等待区块加载超时
    NoSafePos,                   // 没有安全位置
};

struct Task {
    uint64_t     id_{};                                        // 任务ID
    Player*      player_{nullptr};                             // 玩家
    DimensionPos targetPos_;                                   // 目标位置
    DimensionPos sourcePos_;                                   // 原始位置
    short        scheduleCounter_ = 0;                         // 调度计数器
    TaskState    state_           = TaskState::WaitingProcess; // 任务状态

public:
    Task(Task&)             = delete;
    Task& operator=(Task&)  = delete;
    Task(Task&&)            = default;
    Task& operator=(Task&&) = default;

    explicit Task(uint64_t id, Player* player, DimensionPos targetPos, DimensionPos sourcePos)
    : id_(id),
      player_(player),
      targetPos_(targetPos),
      sourcePos_(sourcePos) {}

    void updateState(TaskState state) { state_ = state; }

    bool operator==(const Task& other) const { return id_ == other.id_; }
};

constexpr short SCHEDULE_COUNTER_MAX = 80; // 调度计数器最大值


class SafeTeleport::SafeTeleportImpl {
    std::unordered_map<uint64_t, Task>        queue_;         // 任务队列
    std::unordered_map<std::string, uint64_t> queueMap_;      // 任务队列映射
    uint64_t                                  idCounter_ = 0; // 唯一ID计数器
    ll::event::ListenerPtr                    listener_;      // 玩家退出事件监听器

    void cancelTask(Task& task) {
        queueMap_.erase(task.player_->getRealName());
        queue_.erase(task.id_);
    }

    void handleFailed(Task& task, std::string const& reason) {
        mc::sendText(*task.player_, reason);
        auto& sou = task.sourcePos_;
        task.player_->teleport(sou.first, sou.second);
    }

    void findPosImpl(Task& task) {
        static const std::vector<string> dangerousBlocks = {"minecraft:lava", "minecraft:flowing_lava"};

        auto& logger      = my_mod::MyMod::getInstance().getSelf().getLogger();
        auto& targetPos   = task.targetPos_;
        auto& dimension   = task.player_->getDimension();
        auto& blockSource = task.player_->getDimensionBlockSource();

        short const startY = mc::GetDimensionMaxHeight(dimension); // 维度最大高度
        short const endY   = mc::GetDimensionMinHeight(dimension); // 维度最小高度

        Block* headBlock = nullptr; // 头部方块
        Block* legBlock  = nullptr; // 腿部方块

        BlockPos currentPos{targetPos.first};
        auto&    currentY = currentPos.y;
        currentY          = startY;

        while (currentY > endY) {
            auto& bl = const_cast<Block&>(blockSource.getBlock(currentPos));

            logger.debug(
                "currentY: {}, bl: {}, headBlock: {}, legBlock: {}",
                currentY,
                currentPos.y,
                bl.getTypeName(),
                headBlock ? headBlock->getTypeName() : "nullptr",
                legBlock ? legBlock->getTypeName() : "nullptr"
            );

            if (std::find(dangerousBlocks.begin(), dangerousBlocks.end(), bl.getTypeName()) == dangerousBlocks.end()
                && !bl.isAir() && headBlock->isAir() && legBlock->isAir()) {
                task.targetPos_.first.y = static_cast<float>(currentY) + 1;
                task.updateState(TaskState::FindedSafePosWaitingProcess);
                break;
            }

            if (!headBlock && !legBlock) {
                headBlock = &bl;
                legBlock  = &bl;
            }

            // 交换
            headBlock = legBlock;
            legBlock  = &bl;
            currentY--;
        }

        task.updateState(TaskState::NoSafePos);
    }

    void handleTask(Task& task) {
        switch (task.state_) {
        case TaskState::WaitingProcess: {
            if (mc::IsChunkFullLoaded(task.targetPos_.first, task.player_->getDimensionBlockSource())) {
                task.updateState(TaskState::ChunkLoadedWaitingProcess);
            } else {
                task.updateState(TaskState::WaitingChunkLoad);
            }
            break;
        }
        case TaskState::WaitingChunkLoad: {
            auto& target = task.targetPos_;

            Vec3 tmp{target.first};
            tmp.y = 320;
            task.player_->teleport(tmp, target.second); // 防止摔死

            if (task.scheduleCounter_ > SCHEDULE_COUNTER_MAX) {
                task.updateState(TaskState::WaitChunkLoadTimeout);
                return;
            }

            if (mc::IsChunkFullLoaded(target.first, task.player_->getDimensionBlockSource())) {
                task.updateState(TaskState::ChunkLoadedWaitingProcess);
            } else {
                task.scheduleCounter_++;
            }
            break;
        }
        case TaskState::ChunkLoadedWaitingProcess: {
            mc::sendText(*task.player_, "区块已加载，正在寻找安全位置..."_tr());
            task.updateState(TaskState::FindingSafePos);
            findPosImpl(task);
            break;
        }
        case TaskState::FindingSafePos: {
            break;
        }
        case TaskState::FindedSafePosWaitingProcess: {
            mc::sendText(*task.player_, "安全位置已找到，正在传送..."_tr());
            task.player_->teleport(task.targetPos_.first, task.targetPos_.second);
            task.updateState(TaskState::TaskCompleted);
            break;
        }
        case TaskState::TaskCompleted: {
            mc::sendText(*task.player_, "传送成功!"_tr());
            cancelTask(task);
            break;
        }
        case TaskState::NoSafePos: {
            handleFailed(task, "任务失败，未找到安全坐标"_tr());
            cancelTask(task);
            break;
        }
        case TaskState::WaitChunkLoadTimeout: {
            handleFailed(task, "区块加载超时"_tr());
            cancelTask(task);
            break;
        }
        }
    }

    inline void processTasks() {
        if (queue_.empty()) return;
        for (auto& [id, task] : queue_) {
            handleTask(task);
        }
    }

    inline void init() {
        ll::coro::keepThis([this]() -> ll::coro::CoroTask<> {
            while (GlobalRepeatCoroTaskRunning.load()) {
                co_await 10_tick;
                processTasks();
            }
            co_return;
        }).launch(ll::thread::ServerThreadExecutor::getDefault());

        listener_ = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
            [this](ll::event::PlayerDisconnectEvent& ev) {
                auto name = ev.self().getRealName();
                my_mod::MyMod::getInstance().getSelf().getLogger().debug("玩家 {} 退出服务器，传送任务取消", name);

                auto iter = queueMap_.find(name);
                if (iter != queueMap_.end()) {
                    auto id = iter->second;
                    queue_.erase(id);      // 删除任务
                    queueMap_.erase(iter); // 删除映射
                    // TODO: 传送过程中退出，任务销毁，但玩家还在目标位置空中，需要回溯
                }
            }
        );
    }

public:
    SafeTeleportImpl(const SafeTeleportImpl&)            = delete;
    SafeTeleportImpl& operator=(const SafeTeleportImpl&) = delete;
    SafeTeleportImpl(SafeTeleportImpl&&)                 = delete;
    SafeTeleportImpl& operator=(SafeTeleportImpl&&)      = delete;

    explicit SafeTeleportImpl() { init(); }

    virtual ~SafeTeleportImpl() {
        if (listener_) ll::event::EventBus::getInstance().removeListener(listener_);
        queue_.clear();    // 清空任务队列
        queueMap_.clear(); // 清空任务映射
    }

    [[nodiscard]] static SafeTeleportImpl& getInstance() {
        static SafeTeleportImpl instance;
        return instance;
    }

    uint64_t createTask(Player* player, DimensionPos targetPos, DimensionPos sourcePos) {
        auto id = idCounter_++;
        queue_.emplace(id, Task{id, player, targetPos, sourcePos}); // 创建任务
        queueMap_.emplace(player->getRealName(), id);               // 创建映射
        return id;
    }
};


void SafeTeleport::teleportTo(Player& player, Vec3 const& pos, int dimid) {
    if (!Config::cfg.land.landTp) {
        mc::sendText<mc::LogLevel::Info>(player, "此功能已被管理员关闭"_tr());
        return;
    }

    SafeTeleportImpl::getInstance()
        .createTask(&player, {pos, dimid}, {player.getPosition(), player.getDimensionId().id});
}
SafeTeleport& SafeTeleport::getInstance() {
    static SafeTeleport instance;
    return instance;
}


} // namespace land
