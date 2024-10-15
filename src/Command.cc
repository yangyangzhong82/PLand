#include "pland/Command.h"
#include "pland/Global.h"
#include "pland/LandDraw.h"
#include "pland/LandSelector.h"
#include "pland/Particle.h"
#include "pland/utils/MC.h"


#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/service/Bedrock.h"
#include "mc/common/wrapper/optional_ref.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/network/packet/SetTimePacket.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CommandPositionFloat.h"
#include "mc/server/commands/CommandSelector.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/ChunkBlockPos.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Command.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/dimension/Dimension.h"
#include <ll/api/Logger.h>
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/i18n/I18n.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/service/PlayerInfo.h>
#include <ll/api/service/Service.h>
#include <ll/api/utils/HashUtils.h>
#include <mc/entity/utilities/ActorType.h>
#include <mc/enums/GameType.h>
#include <mc/network/packet/LevelChunkPacket.h>
#include <mc/network/packet/TextPacket.h>
#include <mc/server/ServerLevel.h>
#include <mc/server/ServerPlayer.h>
#include <mc/server/commands/CommandOrigin.h>
#include <mc/server/commands/CommandOriginType.h>
#include <mc/server/commands/CommandOutput.h>
#include <mc/server/commands/CommandParameterOption.h>
#include <mc/server/commands/CommandPermissionLevel.h>
#include <mc/server/commands/CommandRegistry.h>
#include <mc/server/commands/CommandSelector.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Player.h>

#include "magic_enum.hpp"

#include "pland/Config.h"
#include "pland/GUI.h"
#include "pland/PLand.h"
#include "pland/utils/Utils.h"

namespace land {

#define CHECK_TYPE(ori, out, type)                                                                                     \
    if (ori.getOriginType() != type) {                                                                                 \
        mc::sendText(out, "此命令仅限 {} 使用!"_tr(magic_enum::enum_name(type)));                                      \
        return;                                                                                                        \
    }


struct Selector3DLand {
    bool is3DLand{false};
};

namespace Lambda {

static auto const Root = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    LandMainGui::impl(player);
};


static auto const Mgr = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    LandOPManagerGui::impl(player);
};


enum class OperatorType : int { Op, Deop };
struct OperatorParam {
    OperatorType            type;
    CommandSelector<Player> player;
};
static auto const Operator = [](CommandOrigin const& ori, CommandOutput& out, OperatorParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    auto& db  = PLand::getInstance();
    auto  pls = param.player.results(ori).data;
    for (auto& pl : *pls) {
        if (!pl) {
            continue;
        }
        auto uid  = pl->getUuid().asString();
        auto name = pl->getRealName();
        if (param.type == OperatorType::Op) {
            if (db.isOperator(uid)) {
                mc::sendText(out, "{} 已经是管理员，请不要重复添加"_tr(name));
            } else {
                db.addOperator(uid);
                mc::sendText(out, "{} 已经被添加为管理员"_tr(name));
            }
        } else {
            if (db.isOperator(uid)) {
                db.removeOperator(uid);
                mc::sendText(out, "{} 已经被移除管理员"_tr(name));
            } else {
                mc::sendText(out, "{} 不是管理员，无需移除"_tr(name));
            }
        }
    }
};


static auto const New = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    ChooseLandDimAndNewLand::impl(player);
};

enum class SetType : int { A, B };
struct SetParam {
    SetType type;
};
static auto const Set = [](CommandOrigin const& ori, CommandOutput& out, SetParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player   = *static_cast<Player*>(ori.getEntity());
    auto& selector = LandSelector::getInstance();
    auto& pos      = player.getPosition();

    bool status =
        param.type == SetType::A ? selector.trySelectPointA(player, pos) : selector.trySelectPointB(player, pos);

    if (status) {
        mc::sendText(out, "已选择点{}"_tr(param.type == SetType::A ? "A" : "B"));
    } else {
        mc::sendText<mc::LogLevel::Error>(out, "您还没有开启开启领地选区，请先使用 /pland new 命令"_tr());
    }
};

static auto const Cancel = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    bool  ok     = LandSelector::getInstance().tryCancel(player);
    if (ok) {
        mc::sendText(out, "已取消新建领地"_tr());
    } else {
        mc::sendText<mc::LogLevel::Error>(out, "您还没有开启开启领地选区，没有什么可以取消的哦~"_tr());
    }
};


static auto const Buy = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    LandBuyGui::impl(player);
};

static auto const Reload = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    if (Config::tryLoad()) {
        mc::sendText(out, "领地系统配置已重新加载"_tr());
    } else {
        mc::sendText(out, "领地系统配置加载失败，请检查配置文件"_tr());
    }
};


enum class DrawType : int {
    Disbale = 0, // 关闭绘制
    Near,        // 绘制附近领地
    Current,     // 绘制当前领地
};
struct DrawParam {
    DrawType type;
};
static auto const Draw = [](CommandOrigin const& ori, CommandOutput& out, DrawParam const& param) {
    if (ori.getOriginType() == CommandOriginType::DedicatedServer) {
        if (param.type == DrawType::Disbale) {
            LandDraw::disable();
            mc::sendText(out, "领地绘制已关闭"_tr());
            return;
        }
    }
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    if (param.type == DrawType::Disbale) {
        LandDraw::disable(player);
        mc::sendText(out, "领地绘制已关闭"_tr());
    } else {
        LandDraw::enable(player, param.type == DrawType::Current);
    }
};


}; // namespace Lambda


bool LandCommand::setup() {
    auto& cmd = ll::command::CommandRegistrar::getInstance().getOrCreateCommand("pland", "PLand 领地系统"_tr());

    // pland reload
    cmd.overload().text("reload").execute(Lambda::Reload);

    // pland 领地GUI
    cmd.overload().execute(Lambda::Root);

    // pland gui 领地管理GUI
    cmd.overload().text("gui").execute(Lambda::Root);

    // pland mgr 领地管理GUI
    cmd.overload().text("mgr").execute(Lambda::Mgr);

    // pland <op/deop> <player> 添加/移除管理员
    cmd.overload<Lambda::OperatorParam>().required("type").required("player").execute(Lambda::Operator);

    // pland new 新建一个领地
    cmd.overload().text("new").execute(Lambda::New);

    // pland set <a/b> 选点a/b
    cmd.overload<Lambda::SetParam>().text("set").required("type").execute(Lambda::Set);

    // pland cancel 取消新建
    cmd.overload().text("cancel").execute(Lambda::Cancel);

    // pland buy 购买
    cmd.overload().text("buy").execute(Lambda::Buy);

    // pland draw <disable|near|current> 开启/关闭领地绘制
    if (Config::cfg.land.setupDrawCommand) {
        cmd.overload<Lambda::DrawParam>().text("draw").required("type").execute(Lambda::Draw);
    }

    return true;
}


} // namespace land