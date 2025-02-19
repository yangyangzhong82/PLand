#pragma once
#ifdef LD_DEVTOOL
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <atomic>
#include <thread>


#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "imfilebrowser.h"


namespace land::devtools {


extern GLFWwindow*       G_Window;              // 全局窗口指针
extern std::thread       G_RenderThread;        // 全局渲染线程指针
extern std::atomic<bool> G_RenderThreadRunning; // 全局渲染线程是否运行

extern void init();

extern void show();

extern void hide();

extern void destroy();


} // namespace land::devtools

#endif // LD_DEVTOOL