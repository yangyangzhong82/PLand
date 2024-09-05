#include "pland/GUI.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "mod/MyMod.h"
#include "pland/Calculate.h"
#include "pland/Config.h"
#include "pland/EconomySystem.h"
#include "pland/Global.h"
#include "pland/LandSelector.h"
#include "pland/utils/MC.h"
#include "pland/utils/Utils.h"
#include <string>


using namespace ll::form;


namespace land {

void ChooseLandDimAndNewLand::impl(Player& player) {
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

            LandSelector::getInstance().tryStartSelect(pl, pl.getDimensionId(), land3D);
            mc::sendText(pl, "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_tr(Config::cfg.selector.tool));
        });
}


void LandMainGui::impl(Player& player) {
    auto fm = createForm();
    fm.setTitle(PLUGIN_NAME + ("| 领地菜单"_tr()));
    fm.setContent("欢迎使用 Pland 领地管理插件\n\n请选择一个功能"_tr());

    fm.appendButton("新建领地", "textures/ui/anvil_icon", [](Player& pl) { ChooseLandDimAndNewLand::send(pl); });
    fm.appendButton("管理领地", "textures/ui/icon_spring", [](Player& pl) {});
    fm.appendButton("领地传送", "textures/ui/icon_recipe_nature", [](Player& pl) {});

    fm.appendButton("关闭", "textures/ui/cancel");
    fm.sendTo(player);
}


void LandBuyGui::impl(Player& player) {
    auto dataPtr = LandSelector::getInstance().getSelector(player);
    if (!dataPtr) {
        mc::sendText<mc::LogLevel::Error>(player, "请先使用 /pland new 来选择领地"_tr());
        return;
    }

    bool is3D = dataPtr->mDraw3D;

    auto& economy = EconomySystem::getInstance();

    int price = Calculate(dataPtr->mPos)
                    .eval(
                        is3D ? Config::cfg.land.bought.threeDimensionl.calculate
                             : Config::cfg.land.bought.twoDimensionl.calculate
                    );

    auto fm = createForm();
    fm.setTitle(PLUGIN_NAME + ("| 购买领地"_tr()));

    fm.setContent("领地类型: {}\n体积: {}x{}x{} {}\n范围: {}\n价格: {}\n{}"_tr(
        is3D ? "3D" : "2D",
        dataPtr->mPos.getDepth(),
        dataPtr->mPos.getWidth(),
        dataPtr->mPos.getHeight(),
        dataPtr->mPos.getVolume(),
        dataPtr->mPos.toString(),
        price,
        economy.getSpendTip(player, price)
    ));

    fm.appendButton("确认购买"_tr(), "textures/ui/realms_green_check", [](Player& pl) {
        
    });
    fm.appendButton("暂存订单"_tr(), "textures/ui/recipe_book_icon", [](Player& pl) {});
    fm.appendButton("放弃订单"_tr(), "textures/ui/cancel", [](Player& pl) {
        LandSelector::getInstance().tryCancelSelect(pl);
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
            SelectorChangeYGui::send(pl);
            return;
        }

        try {
            int startY = std::stoi(start);
            int endY   = std::stoi(end);

            if (startY >= endY) {
                mc::sendText<mc::LogLevel::Fatal>(pl, "请输入正确的Y轴范围, 开始Y轴必须小于结束Y轴"_tr());
                SelectorChangeYGui::send(pl);
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


void LandManagerGui::impl(Player& player) {}
} // namespace land