#pragma once
#include "DevToolApp.h"
#include <memory>


namespace devtool {


class DevToolAppManager final {
    std::unique_ptr<DevToolApp> app_;

    DevToolAppManager() = default;

public:
    [[nodiscard]] static DevToolAppManager& getInstance();

    [[nodiscard]] DevToolApp* getOrCreateApp();

    void initApp();

    void destroyApp();
};

} // namespace devtool
