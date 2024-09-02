#include "mod/MyMod.h"

#include <memory>

#include "ll/api/mod/RegisterHelper.h"


#include "pland/Command.h"
#include "pland/PLand.h"
#include "pland/Particle.h"


namespace my_mod {

static std::unique_ptr<MyMod> instance;

MyMod& MyMod::getInstance() { return *instance; }

bool MyMod::load() {
    getSelf().getLogger().debug("Loading...");

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
