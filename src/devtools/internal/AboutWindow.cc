#ifdef LD_DEVTOOL
#include "devtools/DevTools.h"


namespace land::devtools::internal {


void ShowAboutWindow(bool* open) {
    if (!ImGui::Begin("关于", open)) {
        ImGui::End();
        return;
    }

    ImGui::Text("PLand DevTools - 开发者工具");
    ImGui::Text("此工具用于快捷调试、查看、修改 PLand 插件的运行状态");
    ImGui::Separator();
    ImGui::Text("Built with Dear ImGui %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
    ImGui::End();
}


} // namespace land::devtools::internal
#endif // LD_DEVTOOL