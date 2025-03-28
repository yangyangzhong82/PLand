#ifdef LD_DEVTOOL
#pragma once
#include "devtools/components/base/MenuComponent.h"
#include "devtools/components/base/WindowComponent.h"
#include "imgui.h"


namespace land {


class AboutWindow final : public WindowComponent {
public:
    explicit AboutWindow() : WindowComponent() {}

    void render() override {
        if (!ImGui::Begin("关于", getOpenFlag())) {
            ImGui::End();
            return;
        }
        ImGui::Text("PLand DevTools - 开发者工具");
        ImGui::Text("此工具用于快捷调试、查看、修改 PLand 插件的运行状态\n\n");
        ImGui::Separator();
        ImGui::Text("Built with Dear ImGui %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
        ImGui::End();
    }
};

class AboutMenuItem final : public TextMenuItemComponent {
    std::unique_ptr<AboutWindow> aboutWindow_;

public:
    explicit AboutMenuItem() : TextMenuItemComponent("关于") { aboutWindow_ = std::make_unique<AboutWindow>(); }

    // 重写选中状态，绑定到子窗口
    bool* getSelectFlag() override { return aboutWindow_->getOpenFlag(); }
    bool  isSelected() const override { return aboutWindow_->isOpen(); }

    void tick() override { aboutWindow_->tick(); }
};

class HelpMenu final : public MenuComponent {
public:
    explicit HelpMenu() : MenuComponent("帮助") { this->addItem(std::make_unique<AboutMenuItem>()); }
};


} // namespace land


#endif // LD_DEVTOOL