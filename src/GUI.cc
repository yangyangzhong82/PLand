#include "pland/GUI.h"
#include "fmt/compile.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/i18n/I18n.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/service/PlayerInfo.h"
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
#include "pland/SafeTeleport.h"
#include "pland/utils/JSON.h"
#include "pland/utils/MC.h"
#include "pland/utils/Utils.h"
#include "pland/wrapper/FormEx.h"
#include <algorithm>
#include <string>


using namespace ll::form;


namespace land {

// 独立GUI
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
void LandBuyGui::impl(Player& player) {
    auto dataPtr = LandSelector::getInstance().getSelector(player);
    if (!dataPtr) {
        mc::sendText<mc::LogLevel::Error>(player, "请先使用 /pland new 来选择领地"_tr());
        return;
    }

    if (dataPtr->mBindLandData) {
        LandBuyWithReSelectGui::impl(player); // 重新选择领地，切换到另一个购买界面
        return;
    }

    bool const& is3D = dataPtr->mDraw3D;

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
        int const   height      = data->mPos.getHeight();
        auto const& squareRange = Config::cfg.land.bought.squareRange;
        if ((length < squareRange.min || width < squareRange.min)
            || (length > squareRange.max || width > squareRange.max) || height < squareRange.minHeight) {
            mc::sendText<mc::LogLevel::Error>(
                pl,
                "领地范围不合法, 可用范围: 长宽: {}~{} 最小高度: {}"_tr(
                    squareRange.min,
                    squareRange.max,
                    squareRange.minHeight
                )
            );
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
void LandBuyGui::LandBuyWithReSelectGui::impl(Player& player) {
    auto dataPtr = LandSelector::getInstance().getSelector(player);
    if (!dataPtr) {
        mc::sendText<mc::LogLevel::Error>(player, "请先使用 /pland new 来选择领地"_tr());
        return;
    }

    auto&       economy = EconomySystem::getInstance();
    bool const& is3D    = dataPtr->mDraw3D;


    int const& originalPrice   = dataPtr->mBindLandData->mOriginalBuyPrice; // 原始购买价格
    int const  discountedPrice = Calculate::calculateDiscountPrice(         // 新范围购买价格
        Calculate(dataPtr->mPos)
            .eval(
                is3D ? Config::cfg.land.bought.threeDimensionl.calculate
                     : Config::cfg.land.bought.twoDimensionl.calculate
            ),
        Config::cfg.land.discountRate
    );

    // 计算需补差价 & 退还差价
    int needPay = discountedPrice - originalPrice; // 需补差价
    int refund  = originalPrice - discountedPrice; // 退还差价


    // publish event
    LandRangeChangeBeforeEvent ev(player, dataPtr->mBindLandData, dataPtr->mPos, needPay, refund);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto fm = SimpleFormEx::create();
    fm.setTitle(PLUGIN_NAME + ("| 购买领地 & 重新选区"_tr()));
    fm.setContent("体积: {0}x{1}x{2} = {3}\n范围: {4}\n原购买价格: {5}\n需补差价: {6}\n需退差价: {7}\n{8}"_tr(
        dataPtr->mPos.getDepth(),  // 1
        dataPtr->mPos.getWidth(),  // 2
        dataPtr->mPos.getHeight(), // 3
        dataPtr->mPos.getVolume(), // 4
        dataPtr->mPos.toString(),  // 5
        originalPrice,             // 6
        needPay < 0 ? 0 : needPay, // 7
        refund < 0 ? 0 : refund,   // 8
        needPay > 0 ? economy.getSpendTip(player, discountedPrice) : ""
    ));

    fm.appendButton("确认购买"_tr(), "textures/ui/realms_green_check", [needPay, refund, discountedPrice](Player& pl) {
        auto& eco = EconomySystem::getInstance();
        if (needPay > 0 && eco.get(pl) < needPay) {
            mc::sendText<mc::LogLevel::Error>(pl, "您的余额不足，无法购买"_tr());
            return; // 预检查经济
        }

        auto& db       = PLand::getInstance();
        auto& selector = LandSelector::getInstance();
        auto  data     = selector.getSelector(pl);
        if (!data) {
            return;
        }

        int const   length      = data->mPos.getDepth();
        int const   width       = data->mPos.getWidth();
        int const   height      = data->mPos.getHeight();
        auto const& squareRange = Config::cfg.land.bought.squareRange;
        if ((length < squareRange.min || width < squareRange.min)
            || (length > squareRange.max || width > squareRange.max) || height < squareRange.minHeight) {
            mc::sendText<mc::LogLevel::Error>(
                pl,
                "领地范围不合法, 可用范围: 长宽: {}~{} 最小高度: {}"_tr(
                    squareRange.min,
                    squareRange.max,
                    squareRange.minHeight
                )
            );
            return;
        }

        auto lands = db.getLandAt(data->mPos.mMin_A.toBlockPos(), data->mPos.mMax_B.toBlockPos(), data->mDimid);
        if (!lands.empty()) {
            for (auto& land : lands) {
                if (land->getLandID() == data->mBindLandData->getLandID()) continue; // 跳过自己
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

        // 补差价 & 退还差价
        if (needPay > 0) {
            if (!eco.reduce(pl, needPay)) {
                mc::sendText<mc::LogLevel::Error>(pl, "您的余额不足，无法购买"_tr());
                return;
            }
        } else if (refund > 0) {
            if (!eco.add(pl, refund)) {
                mc::sendText<mc::LogLevel::Error>(pl, "经济系统异常,退还差价失败"_tr());
                return;
            }
        }

        auto landPtr = data->mBindLandData;
        if (landPtr->_setLandPos(data->mPos)) {
            landPtr->mOriginalBuyPrice = discountedPrice; // 保存购买价格

            selector.completeAndRelease(pl); // 释放数据
            db.refreshLandRange(landPtr);    // 刷新领地范围

            mc::sendText<mc::LogLevel::Info>(pl, "领地范围修改成功"_tr());

            LandRangeChangeAfterEvent ev(pl, landPtr, data->mPos, needPay, refund);
            ll::event::EventBus::getInstance().publish(ev);

        } else mc::sendText<mc::LogLevel::Error>(pl, "领地范围修改失败"_tr());
    });
    fm.appendButton("暂存订单"_tr(), "textures/ui/recipe_book_icon"); // close
    fm.appendButton("放弃订单"_tr(), "textures/ui/cancel", [](Player& pl) {
        LandSelector::getInstance().tryCancel(pl);
    });

    fm.sendTo(player);
}
void SelectorChangeYGui::impl(Player& player, std::string const& exception) {
    auto dataPtr = LandSelector::getInstance().getSelector(player);
    if (!dataPtr) {
        return;
    }

    CustomForm fm(PLUGIN_NAME + ("| 确认Y轴范围"_tr()));

    fm.appendLabel("确认选区的Y轴范围\n您可以在此调节Y轴范围，如果不需要修改，请直接点击提交"_tr());

    fm.appendInput("start", "开始Y轴"_tr(), "int", std::to_string(dataPtr->mPos.mMin_A.y));
    fm.appendInput("end", "结束Y轴"_tr(), "int", std::to_string(dataPtr->mPos.mMax_B.y));

    fm.appendLabel(exception);

    fm.sendTo(player, [dataPtr](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res.has_value()) {
            return;
        }

        string start = std::get<string>(res->at("start"));
        string end   = std::get<string>(res->at("end"));

        if (!isNumber(start) || !isNumber(end) || isOutOfRange(start) || isOutOfRange(end)) {
            SelectorChangeYGui::impl(pl, "请输入正确的Y轴范围"_tr());
            return;
        }

        try {
            int startY = std::stoi(start);
            int endY   = std::stoi(end);

            if (startY >= endY) {
                SelectorChangeYGui::impl(pl, "请输入正确的Y轴范围, 开始Y轴必须小于结束Y轴"_tr());
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


// 通用组件Gui
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
template <typename ParentForm>
void ChoosePlayerGui::impl(Player& player, ChoosePlayerCall callback) {
    auto fm = SimpleFormEx::create<LandMainGui, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + ("| 选择玩家"_tr()));

    ll::service::getLevel()->forEachPlayer([callback, &fm](Player& target) {
        fm.appendButton(target.getRealName(), "", [callback, &target](Player& self) { callback(self, target); });
        return true;
    });

    fm.sendTo(player);
}
void EditStringGui::impl(
    Player&          player,
    string const&    title,        // 标题
    string const&    text,         // 提示
    string const&    defaultValue, // 默认值
    EditStringResult callback      // 回调
) {
    CustomForm fm(PLUGIN_NAME + title);
    fm.appendInput("str", text, "string", defaultValue);
    fm.sendTo(player, [callback](Player& pl, CustomFormResult res, FormCancelReason) {
        if (!res.has_value()) {
            return;
        }
        callback(pl, std::get<string>(res->at("str")));
    });
}


// 领地主菜单
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


// 领地管理菜单
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
    fm.appendButton("修改领地名称"_tr(), "textures/ui/book_edit_default", [land](Player& pl) {
        EditLandNameGui::impl(pl, land);
    });
    fm.appendButton("修改领地描述"_tr(), "textures/ui/book_edit_default", [land](Player& pl) {
        EditLandDescGui::impl(pl, land);
    });
    fm.appendButton("领地过户"_tr(), "textures/ui/sidebar_icons/my_characters", [land](Player& pl) {
        EditLandOwnerGui::impl(pl, land);
    });
    fm.appendButton("重新选区"_tr(), "textures/ui/anvil_icon", [land](Player& pl) { ReSelectLandGui::impl(pl, land); });
    fm.appendButton("删除领地"_tr(), "textures/ui/icon_trash", [land](Player& pl) { DeleteLandGui::impl(pl, land); });

    fm.sendTo(player);
}
void LandManagerGui::EditLandPermGui::impl(Player& player, LandDataPtr ptr) {
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
void LandManagerGui::DeleteLandGui::impl(Player& player, LandDataPtr ptr) {
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
void LandManagerGui::EditLandNameGui::impl(Player& player, LandDataPtr ptr) {
    EditStringGui::impl(
        player,
        "修改领地名称"_tr(),
        "请输入新的领地名称"_tr(),
        ptr->getLandName(),
        [ptr](Player& pl, string result) {
            ptr->setLandName(result);
            mc::sendText(pl, "领地名称已更新!"_tr());
        }
    );
}
void LandManagerGui::EditLandDescGui::impl(Player& player, LandDataPtr ptr) {
    EditStringGui::impl(
        player,
        "修改领地描述"_tr(),
        "请输入新的领地描述"_tr(),
        ptr->getLandDescribe(),
        [ptr](Player& pl, string result) {
            ptr->setLandDescribe(result);
            mc::sendText(pl, "领地描述已更新!"_tr());
        }
    );
}
void LandManagerGui::EditLandOwnerGui::impl(Player& player, LandDataPtr ptr) {
    ChoosePlayerGui::impl(player, [ptr](Player& self, Player& target) {
        if (self == target) {
            mc::sendText(self, "不能将领地转让给自己, 左手倒右手哦!"_tr());
            return;
        }

        LandOwnerChangeBeforeEvent ev(self, target, ptr->getLandID());
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        ModalForm fm(
            PLUGIN_NAME + " | 确认转让?"_tr(),
            "您确定要将领地转让给 {} 吗?\n转让后，您将失去此领地的权限。\n此操作不可逆,请谨慎操作!"_tr(
                target.getRealName()
            ),
            "确认"_tr(),
            "返回"_tr()
        );
        fm.sendTo(self, [ptr, &target](Player& self, ModalFormResult const& res, FormCancelReason) {
            if (!res) {
                return;
            }

            if (!(bool)res.value()) {
                LandManagerGui::impl(self, ptr->getLandID());
                return;
            }

            if (ptr->setLandOwner(target.getUuid().asString())) {
                mc::sendText(self, "领地已转让给 {}"_tr(target.getRealName()));
                mc::sendText(
                    target,
                    "您已成功接手来自 \"{}\" 的领地 \"{}\""_tr(self.getRealName(), ptr->getLandName())
                );

                LandOwnerChangeAfterEvent ev(self, target, ptr->getLandID());
                ll::event::EventBus::getInstance().publish(ev);
            } else {
                mc::sendText<mc::LogLevel::Error>(self, "领地转让失败!"_tr());
            }
        });
    });
}
void LandManagerGui::ReSelectLandGui::impl(Player& player, LandDataPtr ptr) {
    ModalForm fm(
        PLUGIN_NAME + " | 重新选区"_tr(),
        "重新选区为完全重新选择领地的范围，非直接扩充/缩小现有领地范围。\n重新选择的价格计算方式为\"新范围价格 — 旧范围价值\"，是否继续？"_tr(
        ),
        "确认"_tr(),
        "返回"_tr()
    );
    fm.sendTo(player, [ptr](Player& self, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        if (!(bool)res.value()) {
            LandManagerGui::impl(self, ptr->getLandID());
            return;
        }
        LandSelector::getInstance().tryReSelect(self, ptr);
    });
}


// 编辑领地成员
void EditLandMemberGui::impl(Player& player, LandDataPtr ptr) {
    auto fm = SimpleFormEx::create<LandManagerGui, BackButtonPos::Upper>(ptr->getLandID());

    fm.appendButton("添加成员", "textures/ui/color_plus", [ptr](Player& self) { AddMemberGui::impl(self, ptr); });

    auto& infos = ll::service::PlayerInfo::getInstance();
    for (auto& member : ptr->getLandMembers()) {
        auto i = infos.fromUuid(UUIDm::fromString(member));
        if (!i) {
            my_mod::MyMod::getInstance().getSelf().getLogger().warn("Failed to get player info of {}", member);
        }

        fm.appendButton(i.has_value() ? i->name : member, [member, ptr](Player& self) {
            RemoveMemberGui::impl(self, ptr, member);
        });
    }

    fm.sendTo(player);
}
void EditLandMemberGui::AddMemberGui::impl(Player& player, LandDataPtr ptr) {
    ChoosePlayerGui::impl<EditLandMemberGui>(player, [ptr](Player& self, Player& target) {
        if (self == target) {
            mc::sendText(self, "不能添加自己为领地成员哦!"_tr());
            return;
        }

        LandMemberChangeBeforeEvent ev(self, target.getUuid().asString(), ptr->getLandID(), true);
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return;
        }

        ModalForm fm(
            PLUGIN_NAME + " | 添加成员"_tr(),
            "您确定要添加 {} 为领地成员吗?"_tr(target.getRealName()),
            "确认"_tr(),
            "返回"_tr()
        );
        fm.sendTo(self, [ptr, &target](Player& self, ModalFormResult const& res, FormCancelReason) {
            if (!res) {
                return;
            }

            if (ptr->isLandMember(target.getUuid().asString())) {
                mc::sendText(self, "该玩家已经是领地成员, 请不要重复添加哦!"_tr());
                return;
            }

            if (ptr->addLandMember(target.getUuid().asString())) {
                mc::sendText(self, "添加成功!"_tr());

                LandMemberChangeAfterEvent ev(self, target.getUuid().asString(), ptr->getLandID(), true);
                ll::event::EventBus::getInstance().publish(ev);
            } else {
                mc::sendText(self, "添加失败!"_tr());
            }
        });
    });
}
void EditLandMemberGui::RemoveMemberGui::impl(Player& player, LandDataPtr ptr, UUIDs member) {
    LandMemberChangeBeforeEvent ev(player, member, ptr->getLandID(), false);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    auto info = ll::service::PlayerInfo::getInstance().fromUuid(UUIDm::fromString(member));
    if (!info) {
        my_mod::MyMod::getInstance().getSelf().getLogger().warn("Failed to get player info of {}", member);
    }

    ModalForm fm(
        PLUGIN_NAME + " | 移除成员"_tr(),
        "您确定要移除成员 \"{}\" 吗?"_tr(info.has_value() ? info->name : member),
        "确认"_tr(),
        "返回"_tr()
    );
    fm.sendTo(player, [ptr, member](Player& self, ModalFormResult const& res, FormCancelReason) {
        if (!res) {
            return;
        }

        if (ptr->removeLandMember(member)) {
            mc::sendText(self, "移除成功!"_tr());

            LandMemberChangeAfterEvent ev(self, member, ptr->getLandID(), false);
            ll::event::EventBus::getInstance().publish(ev);
        } else {
            mc::sendText(self, "移除失败!"_tr());
        }
    });
}


// 领地传送GUI
void LandTeleportGui::impl(Player& player, LandID id) {
    auto land = PLand::getInstance().getLand(id);
    if (!land) {
        mc::sendText<mc::LogLevel::Error>(player, "领地不存在"_tr());
        return;
    }

    SafeTeleport::getInstance().teleportTo(player, land->mPos.mMin_A.toBlockPos(), land->getLandDimid());
}


// 管理员GUI
void LandOPManagerGui::impl(Player& player) {
    auto* db = &PLand::getInstance();
    if (!db->isOperator(player.getUuid().asString())) {
        mc::sendText<mc::LogLevel::Error>(player, "无权限访问此表单"_tr());
        return;
    }

    auto fm = SimpleFormEx::create();

    fm.setTitle(PLUGIN_NAME + " | 领地管理"_tr());
    fm.setContent("请选择您要进行的操作"_tr());

    fm.appendButton("管理脚下领地"_tr(), "textures/ui/free_download", [db](Player& self) {
        auto lands = db->getLandAt(self.getPosition(), self.getDimensionId());
        if (!lands) {
            mc::sendText<mc::LogLevel::Error>(self, "您当前所处位置没有领地"_tr());
            return;
        }
        LandManagerGui::impl(self, lands->getLandID());
    });
    fm.appendButton("管理玩家领地"_tr(), "textures/ui/FriendsIcon", [](Player& self) {
        IChoosePlayerFromDB::impl(self, ManageLandWithPlayer::impl);
    });
    fm.appendButton("管理指定领地"_tr(), "textures/ui/magnifyingGlass", [](Player& self) {
        IChooseLandFromDB::impl(self, "", [](Player& self, LandDataPtr ptr) {
            LandManagerGui::impl(self, ptr->getLandID());
        });
    });

    fm.sendTo(player);
}
void LandOPManagerGui::ManageLandWithPlayer::impl(Player& player, UUIDs const& targetPlayer) {
    IChooseLandFromDB::impl(player, targetPlayer, [](Player& self, LandDataPtr ptr) {
        LandManagerGui::impl(self, ptr->getLandID());
    });
}


void LandOPManagerGui::IChoosePlayerFromDB::impl(Player& player, ChoosePlayerCall callback) {
    auto fm = SimpleFormEx::create<LandOPManagerGui, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + " | 玩家列表"_tr());
    fm.setContent("请选择您要管理的玩家"_tr());

    auto const& db    = PLand::getInstance();
    auto const& infos = ll::service::PlayerInfo::getInstance();
    auto const  lands = db.getLands();

    for (auto const& ptr : lands) {
        auto info = infos.fromUuid(UUIDm::fromString(ptr->getLandOwner()));

        fm.appendButton(info.has_value() ? info->name : ptr->getLandOwner(), [ptr, callback](Player& self) {
            callback(self, ptr->getLandOwner());
        });
    }

    fm.sendTo(player);
}
void LandOPManagerGui::IChooseLandFromDB::impl(Player& player, UUIDs const& target, ChooseLandCall callback) {
    auto fm = SimpleFormEx::create<LandOPManagerGui, BackButtonPos::Upper>();
    fm.setTitle(PLUGIN_NAME + " | 领地列表"_tr());
    fm.setContent("请选择您要管理的领地"_tr());

    auto const& db    = PLand::getInstance();
    auto const& infos = ll::service::PlayerInfo::getInstance();
    auto const  lands = target.empty() ? db.getLands() : db.getLands(target);

    for (auto const& ptr : lands) {
        auto info = infos.fromUuid(UUIDm::fromString(ptr->getLandOwner()));
        fm.appendButton(
            "{}\nID: {}  玩家: {}"_tr(
                ptr->getLandName(),
                ptr->getLandID(),
                info.has_value() ? info->name : ptr->getLandOwner()
            ),
            [callback, ptr](Player& self) { callback(self, ptr); }
        );
    }

    fm.sendTo(player);
}


} // namespace land