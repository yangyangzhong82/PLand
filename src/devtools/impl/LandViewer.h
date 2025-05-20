#pragma once
#include "devtools/components/Canvas.h"
#include "devtools/components/base/MenuComponent.h"
#include "devtools/components/base/WindowComponent.h"
#include "imgui.h"
#include <memory>

namespace land {

class LandViewerWindow final : public WindowComponent {
    std::unique_ptr<Canvas> canvas_;
    int                     dimid_;

public:
    explicit LandViewerWindow() {
        canvas_ = std::make_unique<Canvas>();

        canvas_->setAllowDragging(true);
        canvas_->setShowGrid(true);
        canvas_->setShowMousePosition(true);
    }

    void render() override {
        if (!ImGui::Begin("领地可视化", getOpenFlag())) {
            ImGui::End();
            return;
        }

        if (ImGui::Button("重置")) {}
        ImGui::SameLine();
        ImGui::InputInt("维度", &dimid_);
        // ImGui::SameLine();


        canvas_->render();

        ImGui::End();
    }
};

class LandViewerMenuItem final : public TextMenuItemComponent {
    std::unique_ptr<LandViewerWindow> viewerWindow_;

public:
    explicit LandViewerMenuItem() : TextMenuItemComponent("领地可视化") {
        viewerWindow_ = std::make_unique<LandViewerWindow>();
    }

    // 重写选中状态，绑定到子窗口
    bool* getSelectFlag() override { return viewerWindow_->getOpenFlag(); }
    bool  isSelected() const override { return viewerWindow_->isOpen(); }

    void tick() override { viewerWindow_->tick(); }
};


class LandViewerMenu final : public MenuComponent {
public:
    explicit LandViewerMenu() : MenuComponent("领地可视化") { this->addItem(std::make_unique<LandViewerMenuItem>()); }
};


} // namespace land