#include "mc/deps/core/math/Vec3.h"
#include <string>
#ifdef LD_DEVTOOL
#include "devtools/DevTools.h"
#include "fmt/color.h"
#include "imgui.h"
#include "ll/api/service/PlayerInfo.h"
#include "pland/PLand.h"


namespace land::devtools::internal {

void RenderWindow() {}

void ShowLandCacheWindow(bool* open) {
    if (!ImGui::Begin("领地缓存表", open)) {
        ImGui::End();
        return;
    }

    // [刷新] []
    if (ImGui::Button("刷新")) {
        RenderWindow();
    }
    if (ImGui::Button("搜索")) {
        RenderWindow();
    }
    std::string buf;
    buf.resize(256);
    if (ImGui::InputText("搜索", buf.data(), buf.size())) {}
    ImGui::Text("%s", buf.data());

    ImGui::End();
}


} // namespace land::devtools::internal
#endif // LD_DEVTOOL