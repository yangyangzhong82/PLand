#pragma once
#include "components/IComponent.h"
#include <GLFW/glfw3.h>
#include <concepts>
#include <string>
#include <thread>
#include <vector>

namespace devtool {

class DevToolApp final {
public:
    explicit DevToolApp();
    ~DevToolApp();

    DevToolApp(DevToolApp&)                  = delete;
    DevToolApp& operator=(const DevToolApp&) = delete;

public:
    void show() const;

    void hide() const;

    void appendError(std::string msg);

    template <typename T, typename... Args>
        requires std::derived_from<T, IMenu> && std::is_final_v<T>
    void registerMenu(Args&&... args) {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        menus_.emplace_back(std::move(ptr));
    }

private:
    void render();
    void renderErrors() const;
    void renderMenuBar() const;
    void postTick() const;

    void checkAndUpdateScale();

    void initImGuiAndOpenGlWithGLFW();


    GLFWwindow*                         glfwWindow_{nullptr};
    std::thread                         renderThread_;
    std::atomic<bool>                   renderThreadStopFlag_;
    std::vector<std::string>            errors_;
    std::vector<std::unique_ptr<IMenu>> menus_;
};


namespace internals {

void handleGlfwError(int error, const char* description);

void handleWindowClose(GLFWwindow* window);

} // namespace internals


} // namespace devtool
