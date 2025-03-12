#include "pland/Global.h"
#include "mc/world/actor/player/Player.h"
#include "pland/PLand.h"

namespace land {

std::atomic<bool> GlobalRepeatCoroTaskRunning = true;

std::string GetPlayerLocaleCodeFromSettings(Player& player) {
    if (auto set = PLand::getInstance().getPlayerSettings(player.getUuid().asString()); set) {
        if (set->localeCode == PlayerSettings::SYSTEM_LOCALE_CODE()) {
            return player.getLocaleCode();
        } else if (set->localeCode == PlayerSettings::SERVER_LOCALE_CODE()) {
            return std::string(ll::i18n::getDefaultLocaleCode());
        }
        return set->localeCode;
    }
    return std::string(ll::i18n::getDefaultLocaleCode());
}

} // namespace land