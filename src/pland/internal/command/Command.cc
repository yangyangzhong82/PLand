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

#define GET_AS_PLAYER(ORI) (*static_cast<Player*>(ori.getEntity()))

struct Selector3DLand {
    bool is3DLand{false};
};

namespace handlers {

void open_menu(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = GET_AS_PLAYER(ori);
    gui::LandMainMenuGUI::sendTo(player);
}

void open_admin_mgr(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = GET_AS_PLAYER(ori);
    gui::OperatorManager::sendMainMenu(player);
}

void list_admins(CommandOrigin const& origin, CommandOutput& output) {
    CHECK_TYPE(origin, output, CommandOriginType::DedicatedServer);
    auto& operators = PLand::getInstance().getLandRegistry().getOperators();
    if (operators.empty()) {
        feedback_utils::sendErrorText(output, "当前没有管理员"_tr());
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
    feedback_utils::sendText(output, oss.str());
}

struct AdminActionParam {
    enum class Action { add, remove } action;
    CommandSelector<Player> targets;
};
void admin_add_remove(CommandOrigin const& ori, CommandOutput& out, AdminActionParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    auto& db  = PLand::getInstance().getLandRegistry();
    auto  pls = param.targets.results(ori).data;

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
        if (param.action == AdminActionParam::Action::add) {
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
}


struct NewLandParam {
    enum class NewType : int { Default = 0, SubLand } type{NewType::Default};
};
void new_land(CommandOrigin const& ori, CommandOutput& out, NewLandParam const& param) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());

    switch (param.type) {
    case NewLandParam::NewType::Default: {
        gui::NewLandGUI::sendChooseLandDim(player);
        break;
    }

    case NewLandParam::NewType::SubLand: {
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
}

enum class SetType : int { A, B };
struct SetParam {
    SetType type;
};
void selection_set_point(CommandOrigin const& ori, CommandOutput& out, SetParam const& param) {
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
}

void selection_cancel(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    PLand::getInstance().getSelectorManager()->stopSelection(player);
    feedback_utils::sendText(out, "已取消新建领地"_trl(player.getLocaleCode()));
}


void land_buy(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::Player);
    auto& player = *static_cast<Player*>(ori.getEntity());
    gui::LandBuyGUI::sendTo(player);
}

void reload(CommandOrigin const& ori, CommandOutput& out) {
    CHECK_TYPE(ori, out, CommandOriginType::DedicatedServer);
    if (PLand::getInstance().loadConfig()) {
        ll::event::EventBus::getInstance().publish(events::ConfigReloadEvent{});
        feedback_utils::sendText(out, "领地系统配置已重新加载"_tr());
    } else {
        feedback_utils::sendErrorText(out, "领地系统配置加载失败，请检查配置文件"_tr());
    }
}


enum class DrawType : int { Disable = 0, NearLand, CurrentLand };
struct DrawParam {
    DrawType type;
};
void land_draw(CommandOrigin const& ori, CommandOutput& out, DrawParam const& param) {
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
}

void land_set_teleport_pos(CommandOrigin const& ori, CommandOutput& out) {
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
}

void show_current_land_mgr(CommandOrigin const& ori, CommandOutput& out) {
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
}

}; // namespace handlers


bool LandCommand::setup() {
    auto& h = ll::command::CommandRegistrar::getInstance(false).getOrCreateCommand("pland", "PLand - 领地系统"_tr());
    h.alias("land");

    // pland reload
    h.overload().text("reload").execute(&handlers::reload);

    // pland [gui|menu] 领地GUI
    h.overload().execute(&handlers::open_menu);
    h.overload().text("gui").execute(&handlers::open_menu);
    h.overload().text("menu").execute(&handlers::open_menu);

    // pland mgr 领地管理GUI
    h.overload().text("mgr").execute(&handlers::open_admin_mgr);

    // pland admin list 列出所有管理员
    h.overload().text("admin").text("list").execute(&handlers::list_admins);

    // pland admin <add|remove> <targets> 添加/移除管理员
    h.overload<handlers::AdminActionParam>().text("admin").required("action").required("targets").execute(
        &handlers::admin_add_remove
    );

    // pland new [NewType: type] 新建一个领地
    h.overload<handlers::NewLandParam>().text("new").optional("type").execute(&handlers::new_land);

    // pland this 获取当前领地信息
    h.overload().text("this").execute(&handlers::show_current_land_mgr);

    // pland set <a/b> 选点a/b
    h.overload<handlers::SetParam>().text("set").required("type").execute(&handlers::selection_set_point);

    // pland cancel 取消新建
    h.overload().text("cancel").execute(&handlers::selection_cancel);

    // pland buy 购买
    h.overload().text("buy").execute(&handlers::land_buy);

    // pland draw <disable|near|current> 开启/关闭领地绘制
    if (ConfigProvider::getDrawConfig().enabled) {
        h.overload<handlers::DrawParam>().text("draw").required("type").execute(&handlers::land_draw);
    }

    // pland set teleport_pos 设置传送点
    h.overload().text("set").text("teleport_pos").execute(&handlers::land_set_teleport_pos);

    // todo
    // pland lease info [id] 查看当前/指定领地的租赁信息

    // pland admin lease set_start <timestamp|YYYY-MM-DD HH:mm:ss> [id] 设置领地租赁开始时间

    // pland admin lease set_end <timestamp|YYYY-MM-DD HH:mm:ss> [id] 设置领地租赁结束时间

    // pland admin lease add_time <day|hour|minute|second> <amount> [id] 增加领地租赁时间

    // pland admin lease set_state <active|frozen|expired> [id] 设置领地租赁状态

    // pland admin lease thaw [id] 解除领地租赁冻结 (续费 24h)

    // pland admin lease renew <day|hour|minute|second> <amount> [id] 续费领地租赁

    // pland admin lease recycle [id] 回收领地

    // pland admin lease clean <days> 清理到期超过n天的领地

    // pland admin lease to_bought [id] 将租赁领地转为购买领地


#ifdef LD_DEVTOOL
    // pland devtool
    if (ConfigProvider::isDevToolsEnabled()) {
        h.overload().text("devtool").execute([](CommandOrigin const& ori, CommandOutput&) {
            if (ori.getOriginType() == CommandOriginType::DedicatedServer) {
                auto& mod = PLand::getInstance();
                mod.setDevToolVisible(true);
            }
        });
    }
#endif

#ifdef DEBUG
    h.overload().text("debug").text("dump_selectors").execute([](CommandOrigin const& ori, CommandOutput&) {
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
