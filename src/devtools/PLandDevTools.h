#ifdef LD_DEVTOOL
#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <atomic>
#include <thread>
#include <unordered_map>
#include <unordered_set>


#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "devtools/components/base/MenuComponent.h"
#include <memory>
#include <string>
#include <vector>

namespace land {


class PLandDevTools final {
public:
    explicit PLandDevTools();
    ~PLandDevTools();

    void show();

    void hide();

    void warning(std::string msg);

private:
    void render();
    void _init();           // 初始化 ImGui & Opengl & GLFW
    void _renderMenuBar();  // 渲染菜单栏
    void _updateScale();    // 更新缩放比例
    void _renderWarnings(); // 渲染警告信息

    bool                                        isInited_{false};            // 是否初始化 _init
    GLFWwindow*                                 window_{nullptr};            // 窗口
    std::thread                                 renderThread_;               // 渲染线程
    std::atomic_bool                            renderThreadRunning_{false}; // 渲染线程是否运行
    std::vector<std::unique_ptr<MenuComponent>> menus_;                      // 菜单栏
    std::unordered_set<std::string>             warnings_;                   // 警告信息
};

inline std::unique_ptr<PLandDevTools> g_devtools = nullptr;

} // namespace land


#endif // LD_DEVTOOL