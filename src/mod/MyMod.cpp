#include "mod/MyMod.h"

#include <memory>

#include "ll/api/i18n/I18n.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/utils/SystemUtils.h"

#include "pland/Command.h"
#include "pland/PLand.h"
#include "pland/Particle.h"


namespace my_mod {

static std::unique_ptr<MyMod> instance;

MyMod& MyMod::getInstance() { return *instance; }

bool MyMod::load() {
    auto& logger = getSelf().getLogger();
    if (ll::sys_utils::isStdoutSupportAnsi()) {
        logger.title = fmt::format(fmt::fg(fmt::color::light_green), logger.title);
    }
    logger.info(R"(  _____   _                        _ )");
    logger.info(R"( |  __ \ | |                      | |)");
    logger.info(R"( | |__) || |      __ _  _ __    __| |)");
    logger.info(R"( |  ___/ | |     / _` || '_ \  / _` |)");
    logger.info(R"( | |     | |____| (_| || | | || (_| |)");
    logger.info(R"( |_|     |______|\__,_||_| |_| \__,_|)");
    logger.info(R"(                                     )");
    logger.info("Loading...");

    ll::i18n::load(getSelf().getLangDir());
    land::PLand::getInstance().init();

    return true;
}

bool MyMod::enable() {
    auto& logger = getSelf().getLogger();
    logger.info("Build Time: {}", BUILD_TIME);

    land::LandCommand::setup();
    land::LandSelector::getInstance().init();

    return true;
}

bool MyMod::disable() {
    getSelf().getLogger().debug("Disabling...");
    // Code for disabling the mod goes here.
    return true;
}

} // namespace my_mod

LL_REGISTER_MOD(my_mod::MyMod, my_mod::instance);
