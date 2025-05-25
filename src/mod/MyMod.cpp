#include "mod/MyMod.h"

#include <memory>

#include "ll/api/i18n/I18n.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/utils/SystemUtils.h"

#include "ll/api/io/LogLevel.h"
#include "pland/Command.h"
#include "pland/Config.h"
#include "pland/EventListener.h"
#include "pland/Global.h"
#include "pland/LandScheduler.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/Version.h"

#ifdef LD_TEST
#include "LandEventTest.h"
#endif

#ifdef LD_DEVTOOL
#include "DevToolAppManager.h"
#endif

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
    logger.info("{}", PLAND_VERSION_STRING);

    if (auto res = ll::i18n::getInstance().load(getSelf().getLangDir()); !res) {
        logger.error("Load language file failed, plugin will use default language.");
        res.error().log(logger);
    }

    if (PLAND_VERSION_SNAPSHOT) {
        logger.warn("您当前正在使用开发快照版本，此版本可能某些功能异常、损坏、甚至导致崩溃，请勿在生产环境中使用。");
        logger.warn("You are using a development snapshot version, this version may have some abnormal, broken or even "
                    "crash functions, please do not use it in production environment.");
    }


    land::Config::tryLoad();
    logger.setLevel(land::Config::cfg.logLevel); // set console log level

    land::PLand::getInstance().init();

#ifdef DEBUG
    logger.warn("Debug Mode");
    logger.setLevel(ll::io::LogLevel::Trace);
#endif

    return true;
}

bool MyMod::enable() {
    land::LandCommand::setup();
    land::LandScheduler::setup();
    land::EventListener::setup();

#ifdef LD_TEST
    test::SetupEventListener();
#endif

#ifdef LD_DEVTOOL
    if (land::Config::cfg.internal.devTools) devtool::DevToolAppManager::getInstance().initApp();
#endif

    return true;
}

bool MyMod::disable() {
#ifdef LD_DEVTOOL
    if (land::Config::cfg.internal.devTools) devtool::DevToolAppManager::getInstance().destroyApp();
#endif

    auto& logger = getSelf().getLogger();
    logger.info("Stopping thread and saving data...");
    land::PLand::getInstance().stopThread(); // 请求关闭线程
    logger.debug("[Main] Saving land data...");
    land::PLand::getInstance().save();
    logger.debug("[Main] Land data saved.");

    logger.debug("Stopping coroutine...");
    land::GlobalRepeatCoroTaskRunning = false;

    logger.debug("cleaning up...");
    land::SelectorManager::getInstance().cleanup();
    land::LandScheduler::release();
    land::EventListener::release();

    return true;
}

bool MyMod::unload() { return true; }


} // namespace my_mod

LL_REGISTER_MOD(my_mod::MyMod, my_mod::MyMod::getInstance());
