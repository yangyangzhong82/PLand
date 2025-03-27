#include "fmt/format.h"
#include <filesystem>
#ifdef LD_DEVTOOL
#include "PLandDevTools.h"
#include "devtools/impl/DataMenu.h"
#include "devtools/impl/HelpMenu.h"
#include "imgui.h"
#include "mod/MyMod.h"
#include <thread>


namespace land {

PLandDevTools::PLandDevTools() : renderThreadRunning_(true) {
    this->renderThread_ = std::thread(&PLandDevTools::render, this);

    this->menus_.push_back(std::make_unique<DataMenu>());
    this->menus_.push_back(std::make_unique<HelpMenu>());
}

PLandDevTools::~PLandDevTools() {
    if (this->renderThreadRunning_) {
        this->renderThreadRunning_.store(false);
        if (this->renderThread_.joinable()) {
            this->renderThread_.join();
        }
    }
}

void PLandDevTools::show() {
    if (this->window_) {
        glfwShowWindow(this->window_);
    } else {
        my_mod::MyMod::getInstance().getSelf().getLogger().error("DevTools window not initialized");
    }
}

void PLandDevTools::hide() {
    if (this->window_) glfwHideWindow(this->window_);
}

// init
void HandleGlfwError(int error, const char* description) {
    my_mod::MyMod::getInstance().getSelf().getLogger().error("GLFW Error: {}: {}", error, description);
}

void HandleWindowClose(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    if (g_devtools) {
        g_devtools->hide();
    }
}

void PLandDevTools::_init() {
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
        this->window_ = glfwCreateWindow(
            static_cast<int>(900 * xScale),
            static_cast<int>(560 * yScale),
            "PLand - DevTools",
            nullptr,
            nullptr
        );
        if (!this->window_) {
            my_mod::MyMod::getInstance().getSelf().getLogger().error("Failed to create GLFW window");
            return;
        }

        glfwSetWindowCloseCallback(this->window_, HandleWindowClose); // 设置窗口关闭回调函数
        glfwMakeContextCurrent(this->window_);                        // 设置当前上下文
        glfwSwapInterval(1);                                          // 设置垂直同步

        if (glewInit() != GLEW_OK) {
            my_mod::MyMod::getInstance().getSelf().getLogger().error("Failed to initialize GLEW");

            glfwDestroyWindow(this->window_);
            glfwTerminate(); // 终止GLFW
            this->window_ = nullptr;
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
    ImGui_ImplGlfw_InitForOpenGL(this->window_, true); // ImGui <> GLFW
    ImGui_ImplOpenGL3_Init(glslVersion);               // ImGui <> OpenGL
}

void PLandDevTools::_updateScale() {
    static float prevScale = 0;
    float        xScale, yScale;
    glfwGetWindowContentScale(this->window_, &xScale, &yScale);
    if (prevScale != xScale) {
        prevScale   = xScale;
        ImGuiIO& io = ImGui::GetIO();

        auto fontPath = my_mod::MyMod::getInstance().getSelf().getDataDir() / "fonts" / "font.ttf";
        if (!std::filesystem::exists(fontPath)) {
            this->warning(fmt::format(
                "由于字体文件 ( {} ) 不存在\n这可能导致部分模块字体显示异常\n\n建议下载 maple-font 字体的 "
                "Normal-Ligature CN 版本\n将 \"MapleMonoNormal-CN-Regular.ttf\" 重命名为 font.ttf 放置在上述路径下",
                fontPath.string()
            ));
            fontPath = "C:/Windows/Fonts/msyh.ttc";
        }

        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF(
            fontPath.string().c_str(),
            std::round(15 * xScale),
            nullptr,
            io.Fonts->GetGlyphRangesChineseFull()
        );
        io.Fonts->Build();
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplOpenGL3_CreateFontsTexture();
        auto style = ImGuiStyle();
        style.ScaleAllSizes(xScale);
    }
}

void PLandDevTools::_renderMenuBar() {
    auto                  dockspaceId = ImGui::DockSpaceOverViewport();
    static std::once_flag firstTime;
    std::call_once(firstTime, [&] {
        for (auto& menu : this->menus_) {
            ImGui::DockBuilderDockWindow(menu->getLabel().data(), dockspaceId);
        }
        ImGui::DockBuilderFinish(dockspaceId);
    });

    if (ImGui::BeginMainMenuBar()) {
        for (auto& comp : this->menus_) {
            comp->render();
        }
        ImGui::EndMainMenuBar();
    }
}

void PLandDevTools::warning(std::string msg) { this->warnings_.emplace(std::move(msg)); }

void PLandDevTools::_renderWarnings() {
    if (!this->warnings_.empty()) {
        if (!ImGui::Begin("Warning")) {
            ImGui::End();
            return;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 40, 40, 255));
        for (auto& warning : this->warnings_) {
            ImGui::TextUnformatted(warning.data());
        }
        ImGui::PopStyleColor();
        ImGui::End();
    }
}

void PLandDevTools::render() {
    if (!this->isInited_) {
        this->_init();
        this->isInited_ = true;
    }

    while (/* !glfwWindowShouldClose(this->window_) && */ this->renderThreadRunning_) {
        glfwPollEvents();

        if (!glfwGetWindowAttrib(this->window_, GLFW_VISIBLE)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue; // 窗口不可见时跳过渲染
        }

        _updateScale();

        // 渲染ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        _renderMenuBar();  // 渲染菜单栏
        _renderWarnings(); // 渲染警告

        // 更新组件状态
        for (auto& comp : this->menus_) {
            comp->tick();
        }

        // 渲染
        ImGui::Render();
        int displayW;
        int displayH;
        glfwGetFramebufferSize(this->window_, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(this->window_);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(this->window_);
    this->window_ = nullptr;
    glfwTerminate();
}

} // namespace land


#endif // LD_DEVTOOL