#ifdef LD_DEVTOOL
#include "DevTools.h"
#include "mod/MyMod.h"
#include <thread>


namespace land::devtools {


//---------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------
GLFWwindow*         G_Window      = nullptr; // 全局窗口指针
ImGui::FileBrowser* G_FileBrowser = nullptr; // 全局文件浏览器指针
std::thread         G_RenderThread;          // 全局渲染线程指针
std::atomic<bool>   G_RenderThreadRunning;   // 全局渲染线程是否运行


//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------
extern void RenderDevTools(); // RenderDevTools.cc

void init() {
    G_RenderThreadRunning = true;
    G_RenderThread        = std::thread(&RenderDevTools);
}

void show() {
    if (G_Window) {
        glfwShowWindow(G_Window);
    } else {
        my_mod::MyMod::getInstance().getSelf().getLogger().error("DevTools window not initialized");
    }
}

void hide() {
    if (G_Window) glfwHideWindow(G_Window);
}

void destroy() {
    if (G_RenderThreadRunning) {
        G_RenderThreadRunning = false;
        if (G_RenderThread.joinable()) {
            G_RenderThread.join();
        }
    }
}


} // namespace land::devtools
#endif // LD_DEVTOOL