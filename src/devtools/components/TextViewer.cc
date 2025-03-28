#ifdef LD_DEVTOOL
#pragma once
#include "TextViewer.h"
#include "devtools/components/base/WindowComponent.h"
#include "fmt/color.h"
#include "imgui.h"
#include <format>
#include <string>


namespace land {

TextViewer::TextViewer(std::string data, int windowId)
: WindowComponent(),
  data_(std::move(data)),
  windowId_(windowId) {}

void TextViewer::render() {
    if (!ImGui::Begin(
            fmt::format("[{}] 文本查看器", windowId_).c_str(),
            this->getOpenFlag(),
            ImGuiWindowFlags_NoDocking
        )) {
        ImGui::End();
        return;
    }
    if (ImGui::Button("复制")) {
        ImGui::SetClipboardText(data_.data());
    }
    ImGui::SameLine();
    if (ImGui::Button("关闭")) {
        this->setOpenFlag(false);
    }
    ImGui::TextUnformatted(data_.data());
    ImGui::End();
}


} // namespace land


#endif