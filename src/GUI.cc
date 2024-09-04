#include "pland/GUI.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "pland/Config.h"
#include "pland/Global.h"
#include "pland/LandSelector.h"
#include "pland/utils/MC.h"


using namespace ll::form;


namespace land {

void ChooseLandDimidensionlAndNew::send(Player& player) {
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

            LandSelector::getInstance().tryStartSelect(pl, pl.getDimensionId(), !((bool)res.value()));
            mc::sendText(pl, "选区功能已开启，使用命令 /pland set 或使用 {} 来选择ab点"_tr(Config::cfg.selector.tool));
        });
}


void LandMainGui::send(Player& player) {
    // TODO
}
void LandManagerGui::send(Player& player) {
    // TODO
}


void LandBuyGui::send(Player& player) {
    // TODO
}


} // namespace land