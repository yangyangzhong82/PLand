#pragma once
#include "devtools/components/Canvas.h"
#include "devtools/components/base/MenuComponent.h"
#include "devtools/components/base/WindowComponent.h"
#include "imgui.h"
#include <memory>

namespace land {

class LandViewerWindow final : public Canvas {
public:
    explicit LandViewerWindow() : Canvas() {
        // 设置画布配置
        setAllowDragging(true);
        setShowGrid(true);
        setShowMousePosition(true);

        // TODO: 添加领地可视化功能
        // 添加测试图形，测试4个象限
        setShapeUserData(addLine({10, 10}, {60, 60}), "象限#1 测试线段");
        setShapeUserData(addRectangle({70, 70}, {170, 170}), "象限#1 测试矩形");
        setShapeUserData(addCircle({180, 180}, 50.0f), "象限#1 测试圆形");

        setShapeUserData(addLine({-10, 10}, {-60, 60}), "象限#2 测试线段");
        setShapeUserData(addRectangle({-70, 70}, {-170, 170}), "象限#2 测试矩形");
        setShapeUserData(addCircle({-180, 180}, 50.0f), "象限#2 测试圆形");

        setShapeUserData(addLine({-10, -10}, {-60, -60}), "象限#3 测试线段");
        setShapeUserData(addRectangle({-70, -70}, {-170, -170}), "象限#3 测试矩形");
        setShapeUserData(addCircle({-180, -180}, 50.0f), "象限#3 测试圆形");

        setShapeUserData(addLine({10, -10}, {60, -60}), "象限#4 测试线段");
        setShapeUserData(addRectangle({70, -70}, {170, -170}), "象限#4 测试矩形");
        setShapeUserData(addCircle({180, -180}, 50.0f), "象限#4 测试圆形");
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