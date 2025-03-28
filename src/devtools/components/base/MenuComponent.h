#ifdef LD_DEVTOOL
#pragma once
#include "devtools/components/base/Component.h"
#include <memory>
#include <string>
#include <vector>


namespace land {


class IMenuItemComponent : public RenderComponent {};


class TextMenuItemComponent : public IMenuItemComponent {
protected:
    std::string label_;
    std::string shortCut_;
    bool        enable_{true};
    bool        selected_{false};

public:
    explicit TextMenuItemComponent(std::string label);
    explicit TextMenuItemComponent(std::string label, std::string shortCut);
    explicit TextMenuItemComponent(std::string label, std::string shortCut, bool selected);
    explicit TextMenuItemComponent(std::string label, std::string shortCut, bool selected, bool enable);

    virtual ~TextMenuItemComponent() = default;

    /**
     * @brief 菜单栏显示的名称
     */
    [[nodiscard]] virtual std::string const& getLabel() const;

    /**
     * @brief 菜单的快捷键，例如Ctrl+X
     */
    [[nodiscard]] virtual std::string const& getShortCut() const;

    /**
     * @brief 判断菜单项是否启用
     */
    [[nodiscard]] virtual bool isEnabled() const;

    /**
     * @brief 判断菜单项是否被选中
     */
    [[nodiscard]] virtual bool isSelected() const;

    /**
     * @brief 获取选中标志位
     */
    [[nodiscard]] virtual bool* getSelectFlag();

    /**
     * @brief 获取启用标志位
     */
    [[nodiscard]] virtual bool* getEnableFlag();

    /**
     * @brief 渲染菜单项
     * ImGui::MenuItem
     */
    virtual void render() override;
};


class MenuComponent : public RenderComponent {
protected:
    std::string                                      label_;        // 菜单栏显示的名称
    bool                                             enable_{true}; // 是否启用
    std::vector<std::unique_ptr<IMenuItemComponent>> items_;        // 菜单项

public:
    explicit MenuComponent(std::string label);
    virtual ~MenuComponent() = default;

    /**
     * @brief 菜单栏显示的名称
     */
    [[nodiscard]] virtual std::string const& getLabel() const;

    /**
     * @brief 判断菜单是否启用
     */
    [[nodiscard]] virtual bool isEnable() const;

    /**
     * @brief 获取启用标志位
     */
    [[nodiscard]] virtual bool* getEnableFlag();

    /**
     * @brief 获取菜单项
     */
    [[nodiscard]] virtual std::vector<std::unique_ptr<IMenuItemComponent>> const& getItems() const;

    /**
     * @brief 获取菜单项
     */
    [[nodiscard]] virtual std::vector<std::unique_ptr<IMenuItemComponent>>& getItems();

    /**
     * @brief 添加菜单项
     */
    void addItem(std::unique_ptr<IMenuItemComponent> item);


    /**
     * @brief 渲染菜单栏
     * ImGui::BeginMenu -> IMenuItemComponent::render
     */
    void render() override;

    /**
     * @brief 更新帧
     * IMenuItemComponent::tick
     */
    void tick() override;
};

} // namespace land


#endif // LD_DEVTOOL