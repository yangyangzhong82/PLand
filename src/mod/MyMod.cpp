#include "mod/MyMod.h"

#include <memory>

#include "ll/api/i18n/I18n.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/utils/SystemUtils.h"

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
    logger.info("Build Time: {}", BUILD_TIME);

    ll::i18n::load(getSelf().getLangDir());

    land::Config::tryLoad();
    logger.consoleLevel = land::Config::cfg.logLevel; // set console log level

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
    logger.info("Saveing...");

    land::PLand::getInstance().mThread.request_stop(); // 请求关闭线程
    land::GlobalTickScheduler.clear();
    land::PLand::getInstance().save();
    land::LandSelector::getInstance().uninit();
    land::LandScheduler::release();
    land::EventListener::release();
    land::LandDraw::release();

    return true;
}

} // namespace my_mod

LL_REGISTER_MOD(my_mod::MyMod, my_mod::instance);
