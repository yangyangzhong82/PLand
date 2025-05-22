#include "DevToolAppManager.h"
#include "DevToolApp.h"
#include <stdexcept>

#include "impl/helper/HelperMenu.h"
#include "impl/viewer/DataMenu.h"

namespace devtool {


DevToolAppManager& DevToolAppManager::getInstance() {
    static DevToolAppManager instance;
    return instance;
}

DevToolApp* DevToolAppManager::getOrCreateApp() {
    if (!app_) {
        initApp();
    }
    return app_.get();
}

void DevToolAppManager::initApp() {
    if (app_) {
        throw std::runtime_error("DevToolApp already initialized");
    }
    app_ = std::make_unique<DevToolApp>();

    // 注册菜单
    app_->registerMenu<DataMenu>();
    app_->registerMenu<HelperMenu>();
}

void DevToolAppManager::destroyApp() {
    if (app_) {
        app_.reset();
    }
}


} // namespace devtool