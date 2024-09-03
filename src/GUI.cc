#include "pland/GUI.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "pland/Global.h"
#include "pland/LandSelector.h"
#include "pland/utils/MC.h"

using namespace ll::form;


namespace land {

void ChooseLandDimensionlAndNew::send(Player& player) {
    ModalForm(
        PLUGIN_NAME + ("| Choose Land Dimension"_tr()),
        "Please select a territory dimension\n2D: The territory contains the entire Y-axis\n3D: You can set the Y-axis range by yourself"_tr(
        ),
        "2D", // true
        "3D"  // false
    )
        .sendTo(player, [](Player& pl, ModalFormResult res, FormCancelReason) {
            if (!res.has_value()) {
                return;
            }

            LandSelector::getInstance().tryStartSelect(pl, pl.getDimensionId(), !((bool)res.value()));
            mc::sendText(pl, "Selection is turned on"_tr());
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