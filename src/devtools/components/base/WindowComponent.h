#ifdef LD_DEVTOOL
#pragma once
#include "devtools/components/base/Component.h"


namespace land {

class WindowComponent : public RenderComponent {
protected:
    bool openFlag_{false};

public:
    virtual ~WindowComponent() = default;

    /**
     * @brief 窗口是否已打开
     */
    [[nodiscard]] virtual bool isOpen() const;

    /**
     * @brief 获取窗口打开标志位
     */
    [[nodiscard]] virtual bool* getOpenFlag();

    /**
     * @brief 设置窗口打开状态
     */
    virtual void setOpenFlag(bool isOpen);

    /**
     * @brief 根据窗口是否打开来决定是否渲染
     */
    virtual void tick() override;
};

} // namespace land


#endif // LD_DEVTOOL