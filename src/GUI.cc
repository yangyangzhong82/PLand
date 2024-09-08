#include "pland/GUI.h"
#include "fmt/compile.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/i18n/I18n.h"
#include "mc/world/actor/player/Player.h"
#include "mod/MyMod.h"
#include "pland/Calculate.h"
#include "pland/Config.h"
#include "pland/EconomySystem.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandEvent.h"
#include "pland/LandPos.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/utils/JSON.h"
#include "pland/utils/MC.h"
#include "pland/utils/Utils.h"
#include "pland/wrapper/FormEx.h"
#include <string>


using namespace ll::form;


namespace land {

void ChooseLandDimAndNewLand::impl(Player& player) {
    if (!some(Config::cfg.land.bought.allowDimensions, player.getDimensionId().id)) {
        mc::sendText(player, "你所在的维度无法购买领地"_tr());
        return;
    }

    PlayerAskCreateLandBeforeEvent ev(player);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    ModalForm(
        PLUGIN_NAME + ("| 选择领地维度"_tr()),
        "请选择领地维度\n\n2D: 领地拥有整个Y轴\n3D: 自行选择Y轴范围"_tr(),
        "2D", // true
        "3D"  // false
    )
        .sendTo(player, [](Player& pl, ModalFormResult res, FormCancelReason) {
            if (!res.has_value()) {
                return;
            }

            bool land3D = !((bool)res.value());
            if (land3D && !Config::cfg.land.bought.threeDimensionl.enabled) {
                mc::sendText(pl, "3D领地功能未启用，请联系管理员"_tr());
                return;
            }
            if (!land3D && !Config::cfg.land.bought.twoDimensionl.enabled) {
                mc::sendText(pl, "2D领地功能未启用，请联系管理员"_tr());
                return;
            }

            PlayerAskCreateLandAfterEvent ev(pl, land3D);
            ll::event::EventBus::getInstance().publish(ev);

            LandSelector::getInstance().tryStartSelect(pl, pl.getDimensionId(), land3D);
            mc::sendText(pl, "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_tr(Config::cfg.selector.tool));
        });
}


void LandMainGui::impl(Player& player) {
    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 领地菜单"_tr()));
    fm.setContent("欢迎使用 Pland 领地管理插件\n\n请选择一个功能"_tr());

    fm.appendButton("新建领地", "textures/ui/anvil_icon", [](Player& pl) { ChooseLandDimAndNewLand::impl(pl); });
    fm.appendButton("管理领地", "textures/ui/icon_spring", [](Player& pl) {
        ChooseLandGui::impl<LandMainGui>(pl, LandManagerGui::impl);
    });
    fm.appendButton("领地传送", "textures/ui/icon_recipe_nature", [](Player& pl) {
        ChooseLandGui::impl<LandMainGui>(pl, LandTeleportGui::impl);
    });

    fm.appendButton("关闭", "textures/ui/cancel");
    fm.sendTo(player);
}

template <typename ParentForm>
void ChooseLandGui::impl(Player& player, ChooseCallback callback) {
    auto fm = SimpleFormEx::create<ParentForm, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + ("| 选择领地"_tr()));
    fm.setContent("请选择一个领地"_tr());

    auto lands = PLand::getInstance().getLands(player.getUuid().asString());
    for (auto& land : lands) {
        fm.appendButton(
            fmt::format("{}\n维度: {} | ID: {}", land->getLandName(), land->getLandDimid(), land->getLandID()),
            "textures/ui/icon_recipe_nature",
            [callback, land](Player& pl) { callback(pl, land->getLandID()); }
        );
    }

    fm.sendTo(player);
}

void LandManagerGui::impl(Player& player, LandID id) {
    auto land = PLand::getInstance().getLand(id);
    if (!land) {
        mc::sendText<mc::LogLevel::Error>(player, "领地不存在"_tr());
        return;
    }

    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 领地管理 [{}]"_tr(land->getLandID())));
    fm.setContent("领地: {}\n类型: {}\n大小: {}x{}x{} = {}\n范围: {}\n"_tr(
        land->getLandName(),
        land->is3DLand() ? "3D" : "2D",
        land->getLandPos().getDepth(),
        land->getLandPos().getWidth(),
        land->getLandPos().getHeight(),
        land->getLandPos().getVolume(),
        land->getLandPos().toString()
    ));

    fm.appendButton("编辑权限"_tr(), "textures/ui/sidebar_icons/promotag", [land](Player& pl) {
        EditLandPermGui::impl(pl, land);
    });
    fm.appendButton("修改成员"_tr(), "textures/ui/FriendsIcon", [land](Player& pl) {
        EditLandMemberGui::impl(pl, land);
    });
    fm.appendButton("修改领地名称"_tr(), "textures/ui/book_edit_default", [land](Player& pl) {});
    fm.appendButton("修改领地描述"_tr(), "textures/ui/book_edit_default", [land](Player& pl) {});
    fm.appendButton("领地过户"_tr(), "textures/ui/sidebar_icons/my_characters", [land](Player& pl) {});
    // fm.appendButton("重新选区"_tr(), "textures/ui/anvil_icon", [land](Player& pl) {});
    fm.appendButton("删除领地"_tr(), "textures/ui/icon_trash", [land](Player& pl) { DeleteLandGui::impl(pl, land); });

    fm.sendTo(player);
}

void EditLandMemberGui::impl(Player& player, LandDataPtr ptr) {
    auto fm = SimpleFormEx::create<LandManagerGui>(ptr->getLandID());
    // TODO

    fm.sendTo(player);
}


void EditLandPermGui::impl(Player& player, LandDataPtr ptr) {
    CustomForm fm(PLUGIN_NAME + " | 编辑权限"_tr());

    auto& i18n = ll::i18n::getInstance();

    auto js = JSON::structTojson(ptr->getLandPermTableConst());
    for (auto& [k, v] : js.items()) {
        fm.appendToggle(k, (string)i18n->get(k), v);
    }

    fm.sendTo(player, [ptr](Player& pl, CustomFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        auto& perm = ptr->getLandPermTable();
        auto  j    = JSON::structTojson(perm);
        for (auto const& [key, value] : j.items()) {
            bool const val = std::get<uint64_t>(res->at(key));
            j[key]         = val;
        }

        JSON::jsonToStructNoMerge(j, perm);

        mc::sendText(pl, "权限表已更新");
    });
}

void DeleteLandGui::impl(Player& player, LandDataPtr ptr) {
    int price = Calculate::calculateRefundsPrice(ptr->mOriginalBuyPrice, Config::cfg.land.refundRate);

    PlayerDeleteLandBeforeEvent ev(player, ptr->getLandID(), price);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    ModalForm fm(
        PLUGIN_NAME + " | 确认删除?"_tr(),
        "您确定要删除领地 {} 吗?\n删除领地后，您将获得 {} 金币的退款。\n此操作不可逆,请谨慎操作!"_tr(
            ptr->getLandName(),
            price
        ),
        "确认"_tr(),
        "返回"_tr()
    );
    fm.sendTo(player, [ptr, price](Player& pl, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }
        if (!(bool)res.value()) {
            LandManagerGui::impl(pl, ptr->getLandID());
            return;
        }

        auto& db  = PLand::getInstance();
        auto& eco = EconomySystem::getInstance();

        if (eco.add(pl, price)) {
            db.removeLand(ptr->getLandID());
            mc::sendText(pl, "删除领地成功!"_tr());

            PlayerDeleteLandAfterEvent ev(pl, ptr->getLandID());
            ll::event::EventBus::getInstance().publish(ev);

        } else mc::sendText(pl, "经济系统异常，操作失败"_tr());
    });
}

void LandTeleportGui::impl(Player& player, LandID id) {
    auto land = PLand::getInstance().getLand(id);
    if (!land) {
        mc::sendText<mc::LogLevel::Error>(player, "领地不存在"_tr());
        return;
    }
    // TODO: teleport
}


void LandBuyGui::impl(Player& player) {
    auto dataPtr = LandSelector::getInstance().getSelector(player);
    if (!dataPtr) {
        mc::sendText<mc::LogLevel::Error>(player, "请先使用 /pland new 来选择领地"_tr());
        return;
    }

    bool& is3D = dataPtr->mDraw3D;

    auto& economy = EconomySystem::getInstance();

    int originalPrice = Calculate(dataPtr->mPos)
                            .eval(
                                is3D ? Config::cfg.land.bought.threeDimensionl.calculate
                                     : Config::cfg.land.bought.twoDimensionl.calculate
                            );
    int discountedPrice = Calculate::calculateDiscountPrice(originalPrice, Config::cfg.land.discountRate);

    // publish event
    PlayerBuyLandBeforeEvent ev(player, dataPtr, discountedPrice);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 购买领地"_tr()));
    fm.setContent("领地类型: {}\n体积: {}x{}x{} = {}\n范围: {}\n原价: {}\n折扣价: {}\n{}"_tr(
        is3D ? "3D" : "2D",
        dataPtr->mPos.getDepth(),
        dataPtr->mPos.getWidth(),
        dataPtr->mPos.getHeight(),
        dataPtr->mPos.getVolume(),
        dataPtr->mPos.toString(),
        originalPrice,
        discountedPrice,
        economy.getSpendTip(player, discountedPrice)
    ));

    fm.appendButton("确认购买"_tr(), "textures/ui/realms_green_check", [discountedPrice](Player& pl) {
        auto& eco = EconomySystem::getInstance();
        if (eco.get(pl) < discountedPrice) {
            mc::sendText<mc::LogLevel::Error>(pl, "您的余额不足，无法购买"_tr());
            return; // 预检查经济
        }

        auto& db = PLand::getInstance();
        if ((int)db.getLands(pl.getUuid().asString()).size() >= Config::cfg.land.maxLand) {
            mc::sendText<mc::LogLevel::Error>(pl, "您已经达到最大领地数量"_tr());
            return;
        }

        auto& selector = LandSelector::getInstance();
        auto  data     = selector.getSelector(pl);
        if (!data) {
            return;
        }

        int const   length      = data->mPos.getDepth();
        int const   width       = data->mPos.getWidth();
        auto const& squareRange = Config::cfg.land.bought.squareRange;
        if (length < squareRange.min || width < squareRange.min) {
            mc::sendText<mc::LogLevel::Error>(pl, "领地范围过小，最小为 {}x{}"_tr(squareRange.min, squareRange.min));
            return;
        }
        if (length > squareRange.max || width > squareRange.max) {
            mc::sendText<mc::LogLevel::Error>(pl, "领地范围过大，最大为 {}x{}"_tr(squareRange.max, squareRange.max));
            return;
        }

        auto lands = db.getLandAt(data->mPos.mMin_A.toBlockPos(), data->mPos.mMax_B.toBlockPos(), data->mDimid);
        if (!lands.empty()) {
            for (auto& land : lands) {
                if (LandPos::isCollision(land->mPos, data->mPos)) {
                    mc::sendText<mc::LogLevel::Error>(pl, "领地重叠，请重新选择"_tr());
                    return;
                }
                if (!LandPos::isComplisWithMinSpacing(land->mPos, data->mPos)) {
                    mc::sendText<mc::LogLevel::Error>(pl, "领地距离过近，请重新选择"_tr());
                    return;
                }
            }
        }

        // 扣除经济
        if (!eco.reduce(pl, discountedPrice)) {
            mc::sendText<mc::LogLevel::Error>(pl, "您的余额不足，无法购买"_tr());
            return;
        }

        auto landPtr = selector.makeLandFromSelector(pl);
        if (db.addLand(landPtr)) {
            landPtr->mOriginalBuyPrice = discountedPrice; // 保存购买价格
            selector.completeAndRelease(pl);
            mc::sendText<mc::LogLevel::Info>(pl, "购买成功"_tr());

            PlayerBuyLandAfterEvent ev(pl, landPtr);
            ll::event::EventBus::getInstance().publish(ev);

        } else mc::sendText<mc::LogLevel::Error>(pl, "购买失败"_tr());
    });
    fm.appendButton("暂存订单"_tr(), "textures/ui/recipe_book_icon"); // close
    fm.appendButton("放弃订单"_tr(), "textures/ui/cancel", [](Player& pl) {
        LandSelector::getInstance().tryCancel(pl);
    });

    fm.sendTo(player);
}


void SelectorChangeYGui::impl(Player& player) {
    auto dataPtr = LandSelector::getInstance().getSelector(player);
    if (!dataPtr) {
        return;
    }

    CustomForm fm(PLUGIN_NAME + ("| 确认Y轴范围"_tr()));

    fm.appendLabel("确认选区的Y轴范围\n您可以在此调节Y轴范围，如果不需要修改，请直接点击提交"_tr());

    fm.appendInput("start", "开始Y轴"_tr(), "int", std::to_string(dataPtr->mPos.mMin_A.y));
    fm.appendInput("end", "结束Y轴"_tr(), "int", std::to_string(dataPtr->mPos.mMax_B.y));

    fm.sendTo(player, [dataPtr](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res.has_value()) {
            return;
        }

        string start = std::get<string>(res->at("start"));
        string end   = std::get<string>(res->at("end"));

        if (!isNumber(start) || !isNumber(end) || isOutOfRange(start) || isOutOfRange(end)) {
            mc::sendText<mc::LogLevel::Fatal>(pl, "请输入正确的Y轴范围"_tr());
            SelectorChangeYGui::impl(pl);
            return;
        }

        try {
            int startY = std::stoi(start);
            int endY   = std::stoi(end);

            if (startY >= endY) {
                mc::sendText<mc::LogLevel::Fatal>(pl, "请输入正确的Y轴范围, 开始Y轴必须小于结束Y轴"_tr());
                SelectorChangeYGui::impl(pl);
                return;
            }

            dataPtr->mPos.mMin_A.y = startY;
            dataPtr->mPos.mMax_B.y = endY;
            dataPtr->mParticle.draw(pl, true); // force update cache

            mc::sendText(pl, "Y轴范围已修改为 {} ~ {}"_tr(startY, endY));
        } catch (...) {
            mc::sendText<mc::LogLevel::Fatal>(pl, "插件内部错误, 请联系开发者"_tr());
            my_mod::MyMod::getInstance().getSelf().getLogger().error(
                "An exception is caught in {} and user {} enters data: {}, {}",
                __FUNCTION__,
                pl.getRealName(),
                start,
                end
            );
        }
    });
}


void LandOPManagerGui::impl(Player& player) {
    // TODO
}
} // namespace land