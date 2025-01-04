#include "mod/MyMod.h"

#include <memory>

#include "ll/api/i18n/I18n.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/utils/SystemUtils.h"

#include "ll/api/io/LogLevel.h"
#include "pland/Calculate.h"
#include "pland/Command.h"
#include "pland/Config.h"
#include "pland/EventListener.h"
#include "pland/Global.h"
#include "pland/LandDraw.h"
#include "pland/LandScheduler.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/Particle.h"


namespace my_mod {

MyMod& MyMod::getInstance() {
    static MyMod instance;
    return instance;
}

bool MyMod::load() {
    auto& logger = getSelf().getLogger();
    logger.info(R"(  _____   _                        _ )");
    logger.info(R"( |  __ \ | |                      | |)");
    logger.info(R"( | |__) || |      __ _  _ __    __| |)");
    logger.info(R"( |  ___/ | |     / _` || '_ \  / _` |)");
    logger.info(R"( | |     | |____| (_| || | | || (_| |)");
    logger.info(R"( |_|     |______|\__,_||_| |_| \__,_|)");
    logger.info(R"(                                     )");
    logger.info("Loading...");

    auto un_used = ll::i18n::getInstance().load(getSelf().getLangDir());

    land::Config::tryLoad();
    logger.setLevel(land::Config::cfg.logLevel); // set console log level

    land::PLand::getInstance().init();

#ifdef DEBUG
    logger.warn("Debug Mode");
    logger.consoleLevel = 5;
#endif

    return true;
}

bool MyMod::enable() {

    land::LandCommand::setup();
    land::LandSelector::getInstance().init();
    land::LandScheduler::setup();
    land::EventListener::setup();
    land::LandDraw::setup();

    return true;
}

bool MyMod::disable() {
    auto& logger = getSelf().getLogger();
    logger.info("Stopping thread and saving data...");
    land::PLand::getInstance().mThread.request_stop(); // 请求关闭线程
    // land::GlobalTickScheduler.clear();
    logger.debug("[Main] Saving land data...");
    land::PLand::getInstance().save();
    logger.debug("[Main] Land data saved.");
    land::LandSelector::getInstance().uninit();
    land::LandScheduler::release();
    land::EventListener::release();
    land::LandDraw::release();

    return true;
}

} // namespace my_mod

LL_REGISTER_MOD(my_mod::MyMod, my_mod::MyMod::getInstance());
