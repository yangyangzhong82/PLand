#include "TextViewer.h"
#include "fmt/color.h"
#include "imgui.h"
#include <format>
#include <string>


namespace devtool {

TextViewer::TextViewer(std::string data) : data_(std::move(data)) {
    static int NEXT_WINDOW_ID = 0;
    this->windowId_           = NEXT_WINDOW_ID++;
}

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


} // namespace devtool
