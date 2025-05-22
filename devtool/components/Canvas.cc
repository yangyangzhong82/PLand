#include "Canvas.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cmath>

namespace devtool {

// Line
bool Canvas::Line::isHovered(const Vec2& pos) const {
    // 线段与点的距离计算
    float lineLength = std::sqrt(std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2));
    if (lineLength < 0.0001f) return false;

    // 计算点到线段的距离
    float t =
        ((pos.x - start.x) * (end.x - start.x) + (pos.y - start.y) * (end.y - start.y)) / (lineLength * lineLength);

    // 将t限制在[0,1]范围内（确保检查的是线段而不是直线）
    t = std::max(0.0f, std::min(1.0f, t));

    // 计算线段上最近的点
    float nearestX = start.x + t * (end.x - start.x);
    float nearestY = start.y + t * (end.y - start.y);

    // 计算点到线段的距离
    float distance = std::sqrt(std::pow(pos.x - nearestX, 2) + std::pow(pos.y - nearestY, 2));

    // 如果距离小于线宽的一半加上一个小的容差，则认为鼠标悬停在线段上
    return distance <= (thickness / 2.0f + 2.0f);
}
void Canvas::Line::draw(ImDrawList* drawList) const {
    Canvas* canvas =
        static_cast<Canvas*>(ImGui::GetCurrentWindow()->StateStorage.GetVoidPtr(ImGui::GetID("CanvasPtr")));
    if (!canvas) return;

    ImVec2 startPos = canvas->canvasToScreenImVec2({start.x, start.y});
    ImVec2 endPos   = canvas->canvasToScreenImVec2({end.x, end.y});

    // 如果线段被悬停，绘制稍微粗一点的线
    float   drawThickness = hovered ? thickness + 1.0f : thickness;
    ImColor drawColor     = hovered ? ImColor(255, 255, 0) : color;

    drawList->AddLine(startPos, endPos, drawColor, drawThickness);
}

// Rectangle
bool Canvas::Rectangle::isHovered(const Vec2& pos) const {
    if (filled) {
        // 对于填充矩形，检查点是否在矩形内部
        // 确保比较时考虑到min和max可能不是按照预期顺序排列的
        float actualMinX = std::min(min.x, max.x);
        float actualMaxX = std::max(min.x, max.x);
        float actualMinY = std::min(min.y, max.y);
        float actualMaxY = std::max(min.y, max.y);

        return pos.x >= actualMinX && pos.x <= actualMaxX && pos.y >= actualMinY && pos.y <= actualMaxY;
    } else {
        // 对于非填充矩形，检查点是否在矩形边框附近
        float actualMinX = std::min(min.x, max.x);
        float actualMaxX = std::max(min.x, max.x);
        float actualMinY = std::min(min.y, max.y);
        float actualMaxY = std::max(min.y, max.y);

        // 检查是否在左边框附近
        bool nearLeftEdge =
            std::abs(pos.x - actualMinX) <= thickness / 2.0f + 2.0f && pos.y >= actualMinY && pos.y <= actualMaxY;

        // 检查是否在右边框附近
        bool nearRightEdge =
            std::abs(pos.x - actualMaxX) <= thickness / 2.0f + 2.0f && pos.y >= actualMinY && pos.y <= actualMaxY;

        // 检查是否在上边框附近
        bool nearTopEdge =
            std::abs(pos.y - actualMinY) <= thickness / 2.0f + 2.0f && pos.x >= actualMinX && pos.x <= actualMaxX;

        // 检查是否在下边框附近
        bool nearBottomEdge =
            std::abs(pos.y - actualMaxY) <= thickness / 2.0f + 2.0f && pos.x >= actualMinX && pos.x <= actualMaxX;

        return nearLeftEdge || nearRightEdge || nearTopEdge || nearBottomEdge;
    }
}
void Canvas::Rectangle::draw(ImDrawList* drawList) const {
    Canvas* canvas =
        static_cast<Canvas*>(ImGui::GetCurrentWindow()->StateStorage.GetVoidPtr(ImGui::GetID("CanvasPtr")));
    if (!canvas) return;

    // 确保min和max按照正确顺序排列
    float actualMinX = std::min(min.x, max.x);
    float actualMaxX = std::max(min.x, max.x);
    float actualMinY = std::min(min.y, max.y);
    float actualMaxY = std::max(min.y, max.y);

    // 将画布坐标转换为屏幕坐标
    ImVec2 minPos = canvas->canvasToScreenImVec2({actualMinX, actualMinY});
    ImVec2 maxPos = canvas->canvasToScreenImVec2({actualMaxX, actualMaxY});

    ImColor drawColor = hovered ? ImColor(255, 255, 0) : color;

    if (filled) {
        drawList->AddRectFilled(minPos, maxPos, drawColor);
        // 如果被悬停，添加边框
        if (hovered) {
            drawList->AddRect(minPos, maxPos, ImColor(255, 255, 255), 0.0f, ImDrawFlags_None, 1.0f);
        }
    } else {
        float drawThickness = hovered ? thickness + 1.0f : thickness;
        drawList->AddRect(minPos, maxPos, drawColor, 0.0f, ImDrawFlags_None, drawThickness);
    }
}

// Circle
bool Canvas::Circle::isHovered(const Vec2& pos) const {
    // 计算点到圆心的距离
    float distance = std::sqrt(std::pow(pos.x - center.x, 2) + std::pow(pos.y - center.y, 2));

    if (filled) {
        // 对于填充圆，检查点是否在圆内
        return distance <= radius;
    } else {
        // 对于非填充圆，检查点是否在圆边框附近
        // 增加一点容差，使得判定更加宽松
        return std::abs(distance - radius) <= thickness / 2.0f + 3.0f;
    }
}
void Canvas::Circle::draw(ImDrawList* drawList) const {
    Canvas* canvas =
        static_cast<Canvas*>(ImGui::GetCurrentWindow()->StateStorage.GetVoidPtr(ImGui::GetID("CanvasPtr")));
    if (!canvas) return;

    ImVec2 centerPos    = canvas->canvasToScreenImVec2({center.x, center.y});
    float  screenRadius = canvas->getScale() * radius;

    ImColor drawColor = hovered ? ImColor(255, 255, 0) : color;

    if (filled) {
        drawList->AddCircleFilled(centerPos, screenRadius, drawColor);
        // 如果被悬停，添加边框
        if (hovered) {
            drawList->AddCircle(centerPos, screenRadius, ImColor(255, 255, 255), 0, 1.0f);
        }
    } else {
        float drawThickness = hovered ? thickness + 1.0f : thickness;
        drawList->AddCircle(centerPos, screenRadius, drawColor, 0, drawThickness);
    }
}

// Canvas
Canvas::Canvas() {}

Canvas::Vec2 Canvas::screenToCanvas(const Vec2& point) const {
    return {(point.x - origin.x - offset.x) / scale, -((point.y - origin.y - offset.y) / scale)};
}

Canvas::Vec2 Canvas::canvasToScreen(const Vec2& point) const {
    return {point.x * scale + origin.x + offset.x, -point.y * scale + origin.y + offset.y};
}

void Canvas::drawGrid(ImDrawList* drawList, const ImVec2& canvasMin, const ImVec2& canvasMax) {
    if (!flags.showGrid) return;

    // 计算网格的起始和结束位置
    float scaledGridSize = gridSize * scale;

    // 计算原点在屏幕上的位置
    Vec2 originScreen = canvasToScreen({0, 0});

    // 计算从原点开始的网格线位置
    float startX = originScreen.x;
    float startY = originScreen.y;

    // 向左和向上扩展网格线，确保覆盖整个可见区域
    while (startX > canvasMin.x) startX -= scaledGridSize;
    while (startY > canvasMin.y) startY -= scaledGridSize;

    // 计算需要绘制的网格线数量
    int numLinesX = static_cast<int>(std::ceil((canvasMax.x - startX) / scaledGridSize)) + 1;
    int numLinesY = static_cast<int>(std::ceil((canvasMax.y - startY) / scaledGridSize)) + 1;

    // 先绘制普通网格线
    for (int i = 0; i < numLinesX; ++i) {
        float x = startX + i * scaledGridSize;

        // 计算这条线在画布坐标系中的 X 坐标
        float canvasX = (x - origin.x - offset.x) / scale;

        // 判断是否是主要网格线（16的倍数）
        bool isMajor = (static_cast<int>(std::round(canvasX / gridSize)) % 16) == 0;

        // 跳过坐标轴线，稍后单独绘制
        if (std::abs(canvasX) < 0.1f) continue;

        drawList->AddLine(ImVec2(x, canvasMin.y), ImVec2(x, canvasMax.y), isMajor ? gridColorMajor : gridColor);
    }

    for (int i = 0; i < numLinesY; ++i) {
        float y = startY + i * scaledGridSize;

        // 计算这条线在画布坐标系中的 Y 坐标
        float canvasY = -((y - origin.y - offset.y) / scale);

        // 判断是否是主要网格线（16的倍数）
        bool isMajor = (static_cast<int>(std::round(canvasY / gridSize)) % 16) == 0;

        // 跳过坐标轴线，稍后单独绘制
        if (std::abs(canvasY) < 0.1f) continue;

        drawList->AddLine(ImVec2(canvasMin.x, y), ImVec2(canvasMax.x, y), isMajor ? gridColorMajor : gridColor);
    }

    // 单独绘制坐标轴线
    // X轴
    drawList->AddLine(
        ImVec2(canvasMin.x, originScreen.y),
        ImVec2(canvasMax.x, originScreen.y),
        ImColor(255, 0, 0, 200),
        2.0f
    );
    // Y轴
    drawList->AddLine(
        ImVec2(originScreen.x, canvasMin.y),
        ImVec2(originScreen.x, canvasMax.y),
        ImColor(0, 255, 0, 200),
        2.0f
    );

    // 绘制原点（如果在可见区域内）
    if (originScreen.x >= canvasMin.x && originScreen.x <= canvasMax.x && originScreen.y >= canvasMin.y
        && originScreen.y <= canvasMax.y) {
        drawList->AddCircleFilled(ImVec2(originScreen.x, originScreen.y), 4.0f, ImColor(255, 255, 0, 255));
    }
}

void Canvas::updateMousePosition() {
    ImVec2 mouseScreenPos = ImGui::GetMousePos();
    Vec2   mouseScreenVec = {mouseScreenPos.x, mouseScreenPos.y};
    mousePos              = screenToCanvas(mouseScreenVec);
}

void Canvas::checkShapeHover() {
    Shape* oldHoveredShape = hoveredShape;
    hoveredShape           = nullptr;

    // 检查每个形状是否被悬停
    for (auto& shape : shapes) {
        bool wasHovered = shape->hovered;
        shape->hovered  = shape->isHovered(mousePos);

        if (shape->hovered) {
            hoveredShape = shape.get();
        }

        // 如果悬停状态从有到无，触发移出形状事件
        if (wasHovered && !shape->hovered && onShapeLeave) {
            onShapeLeave(shape.get(), mousePos);
        }
    }

    // 如果悬停的形状改变，调用回调
    if (hoveredShape != oldHoveredShape) {
        if (oldHoveredShape && !hoveredShape && onShapeLeave) {
            // 如果之前有悬停的形状，现在没有了，触发移出形状事件
            onShapeLeave(oldHoveredShape, mousePos);
        }

        if (hoveredShape && onShapeHover) {
            onShapeHover(hoveredShape, mousePos);
        }
    }
}
ImVec2 Canvas::canvasToScreenImVec2(const Vec2& point) const { return canvasToScreen(point).toImVec2(); }

void Canvas::render() {
    if (!ImGui::BeginChild(
            "Canvas",
            {0, 0},
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        )) {
        ImGui::EndChild();
        return;
    }

    // 将 Canvas 实例存储在窗口的 StateStorage 中
    ImGui::GetCurrentWindow()->StateStorage.SetVoidPtr(ImGui::GetID("CanvasPtr"), this);

    // 添加工具栏
    if (ImGui::Button("重置")) {
        resetView();
    }
    ImGui::SameLine();
    ImGui::Text("缩放: %.2f", scale);
    ImGui::SameLine();
    if (ImGui::Button("-")) {
        scale = std::max(0.1f, scale - 0.1f);
    }
    ImGui::SameLine();
    if (ImGui::Button("+")) {
        scale += 0.1f;
    }
    ImGui::SameLine(0, 20);

    // 跳转到指定位置
    ImGui::SetNextItemWidth(180);
    ImGui::InputFloat("X", &jumpPos.x, 1.0f);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    ImGui::InputFloat("Y", &jumpPos.y, 1.0f);
    ImGui::SameLine();
    if (ImGui::Button("跳转")) {
        // 计算新的偏移量，使得指定的点位于画布中心
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        offset.x          = -jumpPos.x * scale + canvasSize.x / 2.0f;
        offset.y          = jumpPos.y * scale + canvasSize.y / 2.0f;
    }

    // 获取画布区域（考虑工具栏高度）
    ImVec2 canvasPos  = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasEnd  = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);

    // 设置画布原点
    origin = {canvasPos.x, canvasPos.y};

    // 获取绘图列表
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 创建裁剪区域，确保只在画布区域内绘制
    drawList->PushClipRect(canvasPos, canvasEnd, true);

    // 绘制画布背景
    drawList->AddRectFilled(canvasPos, canvasEnd, ImColor(30, 30, 30));

    // 绘制网格
    drawGrid(drawList, canvasPos, canvasEnd);

    // 检查鼠标是否在画布内
    bool prevMouseInCanvas = mouseInCanvas;
    mouseInCanvas          = ImGui::IsWindowHovered() && ImGui::GetMousePos().x >= canvasPos.x
                 && ImGui::GetMousePos().x <= canvasEnd.x && ImGui::GetMousePos().y >= canvasPos.y
                 && ImGui::GetMousePos().y <= canvasEnd.y;

    // 更新鼠标位置
    updateMousePosition();

    // 处理鼠标进入/离开事件
    if (mouseInCanvas != prevMouseInCanvas) {
        if (mouseInCanvas && onMouseEnter) {
            onMouseEnter(mousePos);
        } else if (!mouseInCanvas && onMouseLeave) {
            onMouseLeave(mousePos);
        }
    }

    // 处理拖动
    if (flags.allowDragging && mouseInCanvas) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            dragging     = true;
            lastMousePos = {ImGui::GetMousePos().x, ImGui::GetMousePos().y};
        }
    }

    if (dragging) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            Vec2 currentMousePos  = {ImGui::GetMousePos().x, ImGui::GetMousePos().y};
            offset.x             += currentMousePos.x - lastMousePos.x;
            offset.y             += currentMousePos.y - lastMousePos.y;
            lastMousePos          = currentMousePos;
        } else {
            dragging = false;
        }
    }

    // 处理缩放
    if (mouseInCanvas && ImGui::GetIO().MouseWheel != 0.0f) {
        // 获取鼠标在缩放前的画布坐标
        Vec2 mouseBeforeZoom = mousePos;

        // 调整缩放比例
        float zoomDelta = ImGui::GetIO().MouseWheel * 0.1f;
        scale           = std::max(0.1f, scale + zoomDelta);

        // 计算鼠标在屏幕上的位置
        ImVec2 mouseScreenPos = ImGui::GetMousePos();

        // 调整偏移量，使鼠标指向的画布点保持不变
        offset.x = mouseScreenPos.x - (mouseBeforeZoom.x * scale) - origin.x;
        offset.y = mouseScreenPos.y - (-mouseBeforeZoom.y * scale) - origin.y;

        // 更新鼠标位置
        updateMousePosition();
    }

    // 检查形状悬停
    checkShapeHover();

    // 绘制所有形状
    for (auto& shape : shapes) {
        shape->draw(drawList);
    }

    // 弹出裁剪区域
    drawList->PopClipRect();

    // 显示鼠标位置
    if (flags.showMousePosition && mouseInCanvas) {
        char buf[64];
        snprintf(buf, sizeof(buf), "X: %.1f, Y: %.1f", mousePos.x, mousePos.y);
        drawList->AddText(ImVec2(canvasPos.x + 10, canvasPos.y + 10), ImColor(255, 255, 255), buf);
    }

    // 如果有悬停的形状，显示其用户数据
    if (hoveredShape && !hoveredShape->userData.empty()) {
        ImVec2 tooltipPos  = ImGui::GetMousePos();
        tooltipPos.x      += 15;
        tooltipPos.y      += 15;

        // 计算文本大小
        ImVec2 textSize = ImGui::CalcTextSize(hoveredShape->userData.c_str());

        // 绘制背景
        drawList->AddRectFilled(
            tooltipPos,
            ImVec2(tooltipPos.x + textSize.x + 10, tooltipPos.y + textSize.y + 10),
            ImColor(50, 50, 50, 230)
        );

        // 绘制边框
        drawList->AddRect(
            tooltipPos,
            ImVec2(tooltipPos.x + textSize.x + 10, tooltipPos.y + textSize.y + 10),
            ImColor(150, 150, 150, 200)
        );

        // 绘制文本
        drawList->AddText(
            ImVec2(tooltipPos.x + 5, tooltipPos.y + 5),
            ImColor(255, 255, 255),
            hoveredShape->userData.c_str()
        );
    }

    // 创建一个不可见的按钮覆盖整个画布区域，以捕获鼠标事件
    ImGui::SetCursorScreenPos(canvasPos);
    ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    ImGui::EndChild();
}

int Canvas::addLine(const Vec2& start, const Vec2& end, float thickness, ImColor color) {
    shapes.push_back(std::make_unique<Line>(start, end, thickness, color));
    return shapes.size() - 1;
}

int Canvas::addRectangle(const Vec2& min, const Vec2& max, float thickness, ImColor color, bool filled) {
    shapes.push_back(std::make_unique<Rectangle>(min, max, thickness, color, filled));
    return shapes.size() - 1;
}

int Canvas::addCircle(const Vec2& center, float radius, float thickness, ImColor color, bool filled) {
    shapes.push_back(std::make_unique<Circle>(center, radius, thickness, color, filled));
    return shapes.size() - 1;
}

void Canvas::removeShape(int index) {
    if (index >= 0 && index < shapes.size()) {
        if (hoveredShape == shapes[index].get()) {
            hoveredShape = nullptr;
        }
        shapes.erase(shapes.begin() + index);
    }
}

void Canvas::clearShapes() {
    hoveredShape = nullptr;
    shapes.clear();
}

void Canvas::setShapeUserData(int index, const std::string& data) {
    if (index >= 0 && index < shapes.size()) {
        shapes[index]->userData = data;
    }
}

std::string Canvas::getShapeUserData(int index) const {
    if (index >= 0 && index < shapes.size()) {
        return shapes[index]->userData;
    }
    return "";
}

void Canvas::resetView() {
    jumpPos = {0, 0};
    offset  = {0, 0};
    scale   = 1.0f;
}

} // namespace devtool
