#pragma once
#ifdef LD_DEVTOOL
#include "imgui.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>


namespace land {

class Canvas {
public:
    struct Vec2 {
        float x, y;

        Vec2() : x(0), y(0) {}
        Vec2(float x_, float y_) : x(x_), y(y_) {}

        bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }

        bool operator!=(const Vec2& other) const { return !(*this == other); }

        ImVec2 toImVec2() const { return {x, y}; }
    };

    enum class ShapeType { Line, Rectangle, Circle };

    struct Shape {
        ShapeType   type;
        ImColor     color;
        std::string userData;
        bool        hovered = false;

        Shape(ShapeType t, ImColor c) : type(t), color(c) {}

        virtual ~Shape() = default;

        virtual bool isHovered(const Vec2& pos) const = 0;
        virtual void draw(ImDrawList* drawList) const = 0;
    };

    // 线段
    struct Line : public Shape {
        Vec2  start;
        Vec2  end;
        float thickness;

        Line(const Vec2& s, const Vec2& e, float t, ImColor c)
        : Shape(ShapeType::Line, c),
          start(s),
          end(e),
          thickness(t) {}

        bool isHovered(const Vec2& pos) const override;
        void draw(ImDrawList* drawList) const override;
    };

    // 矩形
    struct Rectangle : public Shape {
        Vec2  min;
        Vec2  max;
        float thickness;
        bool  filled;

        Rectangle(const Vec2& min_, const Vec2& max_, float t, ImColor c, bool f = false)
        : Shape(ShapeType::Rectangle, c),
          min(min_),
          max(max_),
          thickness(t),
          filled(f) {}

        bool isHovered(const Vec2& pos) const override;
        void draw(ImDrawList* drawList) const override;
    };

    // 圆形
    struct Circle : public Shape {
        Vec2  center;
        float radius;
        float thickness;
        bool  filled;

        Circle(const Vec2& c, float r, float t, ImColor c_, bool f = false)
        : Shape(ShapeType::Circle, c_),
          center(c),
          radius(r),
          thickness(t),
          filled(f) {}

        bool isHovered(const Vec2& pos) const override;
        void draw(ImDrawList* drawList) const override;
    };

    // 回调函数类型
    using MouseCallback = std::function<void(const Vec2&)>;
    using ShapeCallback = std::function<void(const Shape*, const Vec2&)>;

    // Canvas 配置标志
    struct Flags {
        bool allowDragging     = true;
        bool showGrid          = true;
        bool showMousePosition = true;
    };

private:
    // 画布属性
    Vec2  origin        = {0, 0}; // 画布原点（相对于窗口）
    Vec2  offset        = {0, 0}; // 画布偏移量（用于拖动）
    float scale         = 1.0f;   // 缩放比例
    bool  dragging      = false;  // 是否正在拖动
    Vec2  lastMousePos  = {0, 0}; // 上一次鼠标位置
    Vec2  mousePos      = {0, 0}; // 当前鼠标位置（画布坐标系）
    bool  mouseInCanvas = false;  // 鼠标是否在画布内

    // 形状存储
    std::vector<std::unique_ptr<Shape>> shapes;
    Shape*                              hoveredShape = nullptr; // 当前鼠标悬停的形状

    // 回调函数
    MouseCallback onMouseEnter = nullptr;
    MouseCallback onMouseLeave = nullptr;
    ShapeCallback onShapeHover = nullptr;
    ShapeCallback onShapeLeave = nullptr;

    // 配置标志
    Flags flags;

    // 网格属性
    float   gridSize       = 20.0f;
    ImColor gridColor      = ImColor(60, 60, 60, 100);
    ImColor gridColorMajor = ImColor(100, 100, 100, 120);

    // 工具函数
    Vec2   screenToCanvas(const Vec2& point) const;
    Vec2   canvasToScreen(const Vec2& point) const;
    void   drawGrid(ImDrawList* drawList, const ImVec2& canvasMin, const ImVec2& canvasMax);
    void   updateMousePosition();
    void   checkShapeHover();
    ImVec2 canvasToScreenImVec2(const Vec2& point) const;

public:
    Canvas();

    // 渲染函数
    void render();

    // 形状绘制函数
    int addLine(const Vec2& start, const Vec2& end, float thickness = 1.0f, ImColor color = ImColor(255, 255, 255));
    int addRectangle(
        const Vec2& min,
        const Vec2& max,
        float       thickness = 1.0f,
        ImColor     color     = ImColor(255, 255, 255),
        bool        filled    = false
    );
    int addCircle(
        const Vec2& center,
        float       radius,
        float       thickness = 1.0f,
        ImColor     color     = ImColor(255, 255, 255),
        bool        filled    = false
    );

    // 形状操作
    void        removeShape(int index);
    void        clearShapes();
    void        setShapeUserData(int index, const std::string& data);
    std::string getShapeUserData(int index) const;

    // 回调设置
    void setOnMouseEnter(MouseCallback callback) { onMouseEnter = callback; }
    void setOnMouseLeave(MouseCallback callback) { onMouseLeave = callback; }
    void setOnShapeHover(ShapeCallback callback) { onShapeHover = callback; }
    void setOnShapeLeave(ShapeCallback callback) { onShapeLeave = callback; }

    // 配置设置
    void setAllowDragging(bool allow) { flags.allowDragging = allow; }
    void setShowGrid(bool show) { flags.showGrid = show; }
    void setShowMousePosition(bool show) { flags.showMousePosition = show; }
    void setGridSize(float size) { gridSize = size; }

    // 获取当前鼠标位置（画布坐标系）
    Vec2 getMousePosition() const { return mousePos; }

    // 缩放和平移
    void  setScale(float newScale) { scale = newScale; }
    float getScale() const { return scale; }
    void  setOffset(const Vec2& newOffset) { offset = newOffset; }
    Vec2  getOffset() const { return offset; }
    void  resetView();
};

} // namespace land

#endif