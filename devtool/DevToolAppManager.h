#pragma once
#include <memory>

namespace devtool {

class DevToolApp;

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
