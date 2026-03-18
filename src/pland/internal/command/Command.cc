#include "magic_enum.hpp"

#include "Command.h"
#include "pland/PLand.h"
#include "pland/drawer/DrawHandleManager.h"
#include "pland/events/domain/ConfigReloadEvent.h"
#include "pland/gui/LandBuyGUI.h"
#include "pland/gui/LandMainMenuGUI.h"
#include "pland/gui/LandManagerGUI.h"
#include "pland/gui/NewLandGUI.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/selector/SelectorManager.h"
#include "pland/service/LandManagementService.h"
#include "pland/service/ServiceLocator.h"
#include "pland/utils/FeedbackUtils.h"

#include "ll/api/command/Command.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/io/Logger.h"
#include "ll/api/service/PlayerInfo.h"


#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/utility/optional_ref.h"
#include "mc/platform/UUID.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandSelector.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/agent/agent_commands/Command.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "pland/gui/admin/OperatorManager.h"

#include <memory>
#include <sstream>
#include <vector>


namespace land::internal {

#define CHECK_TYPE(ori, out, type)                                                                                     \
    if (ori.getOriginType() != type) {                                                                                 \
        feedback_utils::sendErrorText(out, "此命令仅限 {} 使用!"_tr(magic_enum::enum_name(type)));                     \
        return;                                                                                                        \
    }


struct Selector3DLand {
    bool is3DLand{false};
};

namespace Lambda {

static auto const Root = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    gui::LandMainMenuGUI::sendTo(player);
};


static auto const Mgr = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    gui::OperatorManager::sendMainMenu(player);
};


enum class OperatorType : int { Op, Deop };
struct OperatorParam {
    OperatorType            type;
    CommandSelector<Player> player;
};
static auto const Operator = [](CommandOrigin const& ori, CommandOutput& out, OperatorParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    auto& db  = PLand::getInstance().getLandRegistry();
    auto  pls = param.player.results(ori).data;

    if (pls->empty()) {
        feedback_utils::sendErrorText(out, "未找到任何玩家，请确保玩家在线后重试"_tr());
        return;
    }

    for (auto& pl : *pls) {
        if (!pl) {
            continue;
        }
        auto& uid  = pl->getUuid();
        auto  name = pl->getRealName();
        if (param.type == OperatorType::Op) {
            if (db.isOperator(uid)) {
                feedback_utils::sendErrorText(out, "{} 已经是管理员，请不要重复添加"_tr(name));
            } else {
                if (db.addOperator(uid)) feedback_utils::sendText(out, "{} 已经被添加为管理员"_tr(name));
            }
        } else {
            if (db.isOperator(uid)) {
                if (db.removeOperator(uid)) feedback_utils::sendText(out, "{} 已经被移除管理员"_tr(name));
            } else {
                feedback_utils::sendErrorText(out, "{} 不是管理员，无需移除"_tr(name));
            }
        }
    }
};

static auto const ListOperator = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    auto& operators = PLand::getInstance().getLandRegistry().getOperators();
    if (operators.empty()) {
        feedback_utils::sendErrorText(out, "当前没有管理员"_tr());
        return;
    }

    std::ostringstream oss;
    oss << "管理员: "_tr();
    auto& infoDb = ll::service::PlayerInfo::getInstance();
    for (auto& oper : operators) {
        auto info = infoDb.fromUuid(oper);
        if (info) {
            oss << info->name;
        } else {
            oss << oper.asString();
        }
        oss << " | ";
    }
    feedback_utils::sendText(out, oss.str());
    return;
};


enum class NewType : int { Default = 0, SubLand };
struct NewParam {
    NewType type = NewType::Default;
};
static auto const New = [](CommandOrigin const& ori, CommandOutput& out, NewParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    switch (param.type) {
    case NewType::Default: {
        gui::NewLandGUI::sendChooseLandDim(player);
        break;
    }

    case NewType::SubLand: {
        auto expected =
            PLand::getInstance().getServiceLocator().getLandManagementService().requestCreateSubLand(player);
        if (!expected) {
            feedback_utils::sendError(player, expected.error());
            return;
        }
        feedback_utils::sendText(
            player,
            "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_trl(
                player.getLocaleCode(),
                ConfigProvider::getSelectionConfig().alias
            )
        );
        break;
    }
    }
};

enum class SetType : int { A, B };
struct SetParam {
    SetType type;
};
static auto const Set = [](CommandOrigin const& ori, CommandOutput& out, SetParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    if (!PLand::getInstance().getSelectorManager()->hasSelector(player)) {
        feedback_utils::sendErrorText(
            out,
            "您还没有开启领地选区，请先使用 /pland new 命令"_trl(player.getLocaleCode())
        );
        return;
    }

    auto pos = player.getFeetBlockPos();

    auto selector = PLand::getInstance().getSelectorManager()->getSelector(player);
    if (param.type == SetType::A) {
        selector->setPointA(pos);
    } else {
        selector->setPointB(pos);
    }
};

static auto const Cancel = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    PLand::getInstance().getSelectorManager()->stopSelection(player);
    feedback_utils::sendText(out, "已取消新建领地"_trl(player.getLocaleCode()));
};


static auto const Buy = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    gui::LandBuyGUI::sendTo(player);
};

static auto const Reload = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    if (PLand::getInstance().loadConfig()) {
        ll::event::EventBus::getInstance().publish(events::ConfigReloadEvent{});
        feedback_utils::sendText(out, "领地系统配置已重新加载"_tr());
    } else {
        feedback_utils::sendErrorText(out, "领地系统配置加载失败，请检查配置文件"_tr());
    }
};


enum class DrawType : int { Disable = 0, NearLand, CurrentLand };
struct DrawParam {
    DrawType type;
};
static auto const Draw = [](CommandOrigin const& ori, CommandOutput& out, DrawParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);

    auto& player     = *static_cast<Player*>(ori.getEntity());
    auto  localeCode = player.getLocaleCode();
    auto& db         = PLand::getInstance().getLandRegistry();
    auto  handle     = PLand::getInstance().getDrawHandleManager()->getOrCreateHandle(player);

    switch (param.type) {
    case DrawType::Disable: {
        handle->clearLand();
        feedback_utils::sendText(out, "领地绘制已关闭"_trl(localeCode));
        break;
    }

    case DrawType::CurrentLand: {
        auto land = db.getLandAt(player.getPosition(), player.getDimensionId().id);
        if (!land) {
            feedback_utils::sendErrorText(out, "您当前不在领地内"_trl(localeCode));
            return;
        }
        handle->draw(land, mce::Color::GREEN());
        feedback_utils::sendText(out, "已绘制领地"_trl(localeCode));
        break;
    }

    case DrawType::NearLand: {
        auto lands =
            db.getLandAt(player.getPosition(), ConfigProvider::getDrawConfig().range, player.getDimensionId().id);
        for (auto& land : lands) handle->draw(land, mce::Color::WHITE());
        feedback_utils::sendText(out, "已绘制附近 {} 个领地"_trl(localeCode, lands.size()));
        break;
    }
    }
};

static auto const SetLandTeleportPos = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player     = *static_cast<Player*>(ori.getEntity());
    auto  localeCode = player.getLocaleCode();

    auto& mod      = PLand::getInstance();
    auto& registry = mod.getLandRegistry();
    auto  point    = player.getPosition();
    auto  land     = registry.getLandAt(point, player.getDimensionId().id);
    if (!land) {
        feedback_utils::sendErrorText(out, "您当前不在领地内"_trl(localeCode));
        return;
    }

    auto& service = mod.getServiceLocator().getLandManagementService();
    if (auto res = service.setLandTeleportPos(player, land, point)) {
        feedback_utils::notifySuccess(player, "传送点已更新为: {}"_trl(localeCode, point.toString()));
    } else {
        feedback_utils::sendError(player, res.error());
    }
};

static auto const This = [](CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player     = *static_cast<Player*>(ori.getEntity());
    auto  localeCode = player.getLocaleCode();

    auto land = PLand::getInstance().getLandRegistry().getLandAt(player.getPosition(), player.getDimensionId());
    if (!land) {
        feedback_utils::sendText(player, "当前位置没有领地"_trl(localeCode));
        return;
    }

    auto& uuid = player.getUuid();
    if (!land->isOwner(uuid) && !PLand::getInstance().getLandRegistry().isOperator(uuid)) {
        feedback_utils::sendText(player, "当前位置不是你的领地"_trl(localeCode));
        return;
    }

    gui::LandManagerGUI::sendMainMenu(player, land);
};

}; // namespace Lambda


bool LandCommand::setup() {
    auto& cmd =
        ll::command::CommandRegistrar::getInstance(false).getOrCreateCommand("pland", "LandRegistry 领地系统"_tr());

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

    // pland list op
    cmd.overload().text("list").text("op").execute(Lambda::ListOperator);

    // pland new [NewType: type] 新建一个领地
    cmd.overload<Lambda::NewParam>().text("new").optional("type").execute(Lambda::New);

    // pland this 获取当前领地信息
    cmd.overload().text("this").execute(Lambda::This);

    // pland set <a/b> 选点a/b
    cmd.overload<Lambda::SetParam>().text("set").required("type").execute(Lambda::Set);

    // pland cancel 取消新建
    cmd.overload().text("cancel").execute(Lambda::Cancel);

    // pland buy 购买
    cmd.overload().text("buy").execute(Lambda::Buy);

    // pland draw <disable|near|current> 开启/关闭领地绘制
    if (ConfigProvider::getDrawConfig().enabled) {
        cmd.overload<Lambda::DrawParam>().text("draw").required("type").execute(Lambda::Draw);
    }

    // pland set teleport_pos 设置传送点
    cmd.overload().text("set").text("teleport_pos").execute(Lambda::SetLandTeleportPos);

#ifdef LD_DEVTOOL
    // pland devtool
    if (ConfigProvider::isDevToolsEnabled()) {
        cmd.overload().text("devtool").execute([](CommandOrigin const& ori, CommandOutput&) {
            if (ori.getOriginType() == CommandOriginType::DedicatedServer) {
                auto& mod = PLand::getInstance();
                mod.setDevToolVisible(true);
            }
        });
    }
#endif

#ifdef DEBUG
    cmd.overload().text("debug").text("dump_selectors").execute([](CommandOrigin const& ori, CommandOutput&) {
        if (ori.getOriginType() != CommandOriginType::DedicatedServer) {
            return;
        }

        auto& logger = land::PLand::getInstance().getSelf().getLogger();
        land::PLand::getInstance().getSelectorManager()->forEach([&logger](mce::UUID const& uuid, ISelector* selector) {
            logger.debug("Selector: {} - {}", uuid.asString(), selector->dumpDebugInfo());
            return true;
        });
    });
#endif

    return true;
}


} // namespace land::internal
