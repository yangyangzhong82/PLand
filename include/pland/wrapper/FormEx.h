#pragma once
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "mc/world/actor/player/Player.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>

// clang-format off
/*
Usage:

1. 基本用法（无父表单，无额外参数）：
    class SimpleForm : public FormWrapper<SimpleForm> {
    public:
        static void impl(Player& player) {
            auto fm = createForm();
            // 设置表单内容
            fm.sendTo(player);
        }
    };
    SimpleForm::send(player);


2. 带父表单：
    class ChildForm : public FormWrapper<ChildForm, ParentForm> {
    public:
        static void impl(Player& player) {
            auto fm = createForm(); // 自动添加返回按钮
            // 设置表单内容
            fm.sendTo(player);
        }
    };
    ChildForm::send(player);


3. 带自定义父表单回调：
    void customParentCallback(Player& player) {
        // 自定义回调逻辑
    }
    class CustomCallbackForm : public FormWrapper<CustomCallbackForm, ParentForm, ParentCallWrapper<customParentCallback>> {
    public:
        static void impl(Player& player) {
            auto fm = createForm(); // 使用自定义回调
            // 设置表单内容
            fm.sendTo(player);
        }
    };
    CustomCallbackForm::send(player);


4. 带额外参数：
    class ParameterizedForm : public FormWrapper<ParameterizedForm> {
    public:
        static void impl(Player& player, int param1, std::string param2) {
            auto fm = createForm();
            // 使用 param1 和 param2 设置表单内容
            fm.sendTo(player);
        }
    };
    ParameterizedForm::send(player, 42, "Hello");


5. 带回调：
    class CallbackForm : public FormWrapper<CallbackForm> {
    public:
        using Callback = std::function<void(Player&, int)>;

        static void impl(Player& player, Callback callback) {
            auto fm = createForm();
            // 设置表单内容，在适当的地方调用 callback
            fm.sendTo(player, [callback](int buttonId) {
                callback(player, buttonId);
            });
        }
    };
    CallbackForm::send(player, [](Player& p, int result) {
        // 处理回调
    });


6. 特殊(高级)回调:
    // 自定义父表单回调函数
    void customParentCallback(Player& player, int someData) {
        // 自定义回调逻辑
        std::cout << "Custom parent callback with data: " << someData << std::endl;
    }
    // 使用 ParentCallWrapper 包装自定义回调函数
    class CustomCallbackForm : public FormWrapper<CustomCallbackForm, ParentForm, ParentCallWrapper<customParentCallback>> {
    public:
        static void impl(Player& player) {
            auto fm = createForm(); // 这里会使用自定义回调
            // 设置表单内容
            fm.sendTo(player);
        }
    };

    // 或者不同的函数签名
    void anotherParentCallback(Player& player, std::string message, bool flag) {
        // 另一种自定义回调逻辑
    }

    class AnotherCustomForm : public FormWrapper<AnotherCustomForm, ParentForm, ParentCallWrapper<anotherParentCallback>> {
        // ... 实现 ...
    };

    CustomCallbackForm::send(player);


7. 动态父表单(回调):
    class AAA : public FormWrapper<AAA> {
    public:
        using ChooseCallback = std::function<void(Player&, LandID)>;

        template<typename DynamicParentForm, typename DynamicParentCall>
        static void impl(Player& player, ChooseCallback callback) {
            auto fm = createForm<DynamicParentForm, DynamicParentCall>();
            // ...
            fm.sendTo(player, [callback](Player& p, int buttonId) {
                // ...
                callback(p, selectedLand);
            });
        }
    };

    // 在某些情况下，使用 BBB 作为父表单
    AAA::send<BBB>(player, callback);

    // 在其他情况下，使用 CCC 作为父表单
    AAA::send<CCC>(player, callback);

    // 如果不需要父表单，可以不指定或明确指定 void
    AAA::send<void>(player, callback);

*/
// clang-format on


namespace wrapper {

enum class SimpleFormExBack : bool { Upper = true, Lower = false }; // 返回按钮位置


// 由于无法 override SimpleForm::sendTo 方法，因此需要使用一个包装器类
class SimpleFormEx {
public:
    using Callback       = ll::form::SimpleForm::Callback;
    using ButtonCallback = ll::form::SimpleForm::ButtonCallback;
    using BackCallback   = std::function<void(Player&)>;

    SimpleFormEx();
    SimpleFormEx(BackCallback backCallback, SimpleFormExBack pos = SimpleFormExBack::Lower);

    // 移动构造函数和移动赋值运算符
    SimpleFormEx(SimpleFormEx&&) noexcept            = default;
    SimpleFormEx& operator=(SimpleFormEx&&) noexcept = default;

    // 禁用复制构造函数和复制赋值运算符
    SimpleFormEx(const SimpleFormEx&)            = delete;
    SimpleFormEx& operator=(const SimpleFormEx&) = delete;


    SimpleFormEx& setTitle(std::string const& title);
    SimpleFormEx& setContent(std::string const& content);

    SimpleFormEx& appendButton(std::string const& text, ButtonCallback callback = {});
    SimpleFormEx& appendButton(std::string const& text, std::string const& imageData, ButtonCallback callback = {});
    SimpleFormEx& appendButton(
        std::string const& text,
        std::string const& imageData,
        std::string const& imageType,
        ButtonCallback     callback = {}
    );

    SimpleFormEx& sendTo(Player& player, Callback callback = Callback()); // Lower 在 sendTo 时添加

private:
    std::unique_ptr<ll::form::SimpleForm> form;

    bool             mIsUpperAdded{false};                       // 是否已经添加了 Upper 返回按钮
    SimpleFormExBack mSimpleFormExBack{SimpleFormExBack::Lower}; // 返回按钮位置
    BackCallback     mBackCallback{nullptr};                     // 返回按钮回调
};


// 辅助模板来检查类是否有 impl 方法
template <typename T, typename = void>
struct HasImplMethod : std::false_type {};
template <typename T>
struct HasImplMethod<T, std::void_t<decltype(T::impl)>> : std::true_type {};

template <typename T, typename = void>
struct HasCallMethod : std::false_type {};
template <typename T>
struct HasCallMethod<T, std::void_t<decltype(T::call)>> : std::true_type {};


// 表单包装器模板类
template <
    typename Impl,                     // 实现类(必须包含一个 static impl 方法)
    typename DefaultParentForm = void, // 父表单类(可选, 必须继承 FormWrapper)
    typename DefaultParentCall = void, // 父表单的回调函数包装器(可选, 必须继承 ParentCallWrapper)
    auto BackPos               = SimpleFormExBack::Lower // 返回按钮位置(可选)
    >
class FormWrapper {
public:
    template <
        typename DynamicParentForm = DefaultParentForm, // send<B>() => impl<B>() => createForm<B>()
        typename DynamicParentCall = DefaultParentCall, // send<B>() => impl<B>() => createForm<B>()
        typename... Args>
    static void send(Player& player, Args&&... args) {
        static_assert(HasImplMethod<Impl>::value, "Impl must have a static impl method");
        Impl::template impl<DynamicParentForm, DynamicParentCall>(player, std::forward<Args>(args)...);
    }


protected:
    // 创建表单
    template <typename ParentForm = DefaultParentForm, typename ParentCall = DefaultParentCall>
    static SimpleFormEx createForm() {
        if constexpr (std::is_same_v<ParentForm, void>) {
            return SimpleFormEx{}; // 没有父表单
        } else {
            return SimpleFormEx{
                [](Player& p) {
                    if constexpr (!std::is_same_v<ParentCall, void>) {
                        static_assert(HasCallMethod<ParentCall>::value, "ParentCall must have a static call method");
                        ParentCall::call(p);
                    } else {
                        ParentForm::send(p);
                    }
                },
                BackPos
            };
        }
    }
};

// 辅助类处理父表单的回调
// Func: 要包装的回调函数
template <auto Func>
struct ParentCallWrapper {
    // 调用包装的回调函数
    // 支持传递任意数量和类型的参数
    template <typename... Args>
    static void call(Args&&... args) {
        Func(std::forward<Args>(args)...);
    }
};
} // namespace wrapper