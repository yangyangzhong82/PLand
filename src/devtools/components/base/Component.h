#ifdef LD_DEVTOOL
#pragma once


namespace land {

/**
 * @brief 组件基类
 */
class Component {
public:
    virtual ~Component() = default;

    /**
     * @brief 帧更新
     * 组件的帧更新函数，每帧调用一次
     */
    virtual void tick() = 0;

    /**
     * @brief 类型转换
     * @tparam T 转换后的类型
     * @return 转换后的指针
     */
    template <typename T>
    T* As() {
        return dynamic_cast<T*>(this);
    }
};

/**
 * @brief 渲染组件基类
 */
class RenderComponent : public Component {
public:
    virtual ~RenderComponent() = default;

    /**
     * @brief 渲染
     */
    virtual void render() = 0;
};

} // namespace land


#endif // LD_DEVTOOL