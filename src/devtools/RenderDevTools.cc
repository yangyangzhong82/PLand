#ifdef LD_DEVTOOL
#include "GLFW/glfw3.h"
#include "devtools/DevTools.h"
#include "imgui.h"
#include "mod/MyMod.h"
#include "pland/Global.h"


namespace land::devtools {

//---------------------------------------------------------------------------
// 全局处理器
//---------------------------------------------------------------------------
void HandleGlfwError(int error, const char* description) {
    my_mod::MyMod::getInstance().getSelf().getLogger().error("GLFW Error: {}: {}", error, description);
}

void HandleWindowClose(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    hide();
}

//---------------------------------------------------------------------------
// 预声明
//---------------------------------------------------------------------------
namespace internal {
extern void ShowAboutWindow(bool* open);     // internal/AboutWindow.cc
extern void ShowLandCacheWindow(bool* open); // internal/LandCacheWindow.cc
} // namespace internal

//---------------------------------------------------------------------------
// 全局标志位
//---------------------------------------------------------------------------
bool F_ShowAboutWindow     = false;
bool F_ShowLandCacheWindow = false;

void CheckAndHandleFlags() {
    if (F_ShowAboutWindow) {
        internal::ShowAboutWindow(&F_ShowAboutWindow);
    }
    if (F_ShowLandCacheWindow) {
        internal::ShowLandCacheWindow(&F_ShowLandCacheWindow);
    }
}


//---------------------------------------------------------------------------
// 渲染窗口
//---------------------------------------------------------------------------
void RenderDevTools() {
    glfwSetErrorCallback(HandleGlfwError); // 设置GLFW错误回调函数
    if (!glfwInit()) {
        my_mod::MyMod::getInstance().getSelf().getLogger().error("Failed to initialize GLFW");
        return;
    }

    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 设置OpenGL版本
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // 设置窗口默认不可见

    {
        float        xScale, yScale;
        GLFWmonitor* monitor = glfwGetPrimaryMonitor(); // 获取主显示器
        if (!monitor) {
            my_mod::MyMod::getInstance().getSelf().getLogger().error("Failed to get primary monitor");
            return;
        }
        glfwGetMonitorContentScale(monitor, &xScale, &yScale); // 获取主显示器缩放比例
        G_Window = glfwCreateWindow(
            static_cast<int>(600 * xScale),
            static_cast<int>(360 * yScale),
            "PLand - DevTools",
            nullptr,
            nullptr
        );
        if (!G_Window) {
            my_mod::MyMod::getInstance().getSelf().getLogger().error("Failed to create GLFW window");
            return;
        }

        glfwSetWindowCloseCallback(G_Window, HandleWindowClose); // 设置窗口关闭回调函数
        glfwMakeContextCurrent(G_Window);                        // 设置当前上下文
        glfwSwapInterval(1);                                     // 设置垂直同步

        if (glewInit() != GLEW_OK) {
            my_mod::MyMod::getInstance().getSelf().getLogger().error("Failed to initialize GLEW");

            glfwDestroyWindow(G_Window);
            glfwTerminate(); // 终止GLFW
            G_Window = nullptr;
            return;
        }
    }

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io     = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 启用键盘导航
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // 启用Docking
    ImGui::StyleColorsDark();

    // 初始化ImGui与GLFW的绑定
    ImGui_ImplGlfw_InitForOpenGL(G_Window, true); // ImGui <> GLFW
    ImGui_ImplOpenGL3_Init(glslVersion);          // ImGui <> OpenGL

    // 线程主循环
    while (/* !glfwWindowShouldClose(G_Window) && */ G_RenderThreadRunning) {
        glfwPollEvents();

        if (!glfwGetWindowAttrib(G_Window, GLFW_VISIBLE)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue; // 窗口不可见时跳过渲染
        }

        static float prevScale = 0;
        float        xScale, yScale;
        glfwGetWindowContentScale(G_Window, &xScale, &yScale);
        if (prevScale != xScale) {
            prevScale = xScale;

            auto fontPath = "C:/Windows/Fonts/msyh.ttc";
            io.Fonts->Clear();
            io.Fonts
                ->AddFontFromFileTTF(fontPath, std::round(15 * xScale), nullptr, io.Fonts->GetGlyphRangesChineseFull());
            io.Fonts->Build();
            ImGui_ImplOpenGL3_DestroyFontsTexture();
            ImGui_ImplOpenGL3_CreateFontsTexture();
            auto style = ImGuiStyle();
            style.ScaleAllSizes(xScale);
        }

        // 渲染ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 创建DockSpace(用于布局)
        auto                  dockspaceId = ImGui::DockSpaceOverViewport();
        static std::once_flag firstTime;
        std::call_once(firstTime, [&] {
            ImGui::DockBuilderDockWindow("领地缓存表", dockspaceId);
            ImGui::DockBuilderDockWindow("关于", dockspaceId);
            ImGui::DockBuilderFinish(dockspaceId);
        });

        // 菜单栏
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("文件")) {
                ImGui::Text("TODO: 待实现");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("查看")) {
                ImGui::MenuItem("领地缓存表", nullptr, &F_ShowLandCacheWindow);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("帮助")) {
                ImGui::MenuItem("关于", nullptr, &F_ShowAboutWindow);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        CheckAndHandleFlags(); // 处理标志

        // 渲染
        ImGui::Render();
        int displayW;
        int displayH;
        glfwGetFramebufferSize(G_Window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(G_Window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(G_Window);
    G_Window = nullptr;
    glfwTerminate();
}


} // namespace land::devtools
#endif // LAND_DEVTOOL