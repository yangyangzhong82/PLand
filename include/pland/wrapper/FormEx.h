#pragma once
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"
#include "mc/world/actor/player/Player.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>


/*
Usage:

// 基础的 SimpleFormEx
class MyForm : public FormWrapper<MyForm> {
public:
    static void impl(Player& player) {
        auto fm = createForm();
        // do something
        fm.sendTo(player);
    }
};
MyForm::send(player);

// 带有返回按钮的 SimpleFormEx
class MyForm2 : public FormWrapper<MyForm2, ParentForm> {
public:
    static void impl(Player& player) {
        auto fm = createForm();
        // do something
        fm.sendTo(player);
    }
};
MyForm2::send(player);

// 带有返回按钮的 SimpleFormEx(call 自定义表单)
class MyForm3 : public FormWrapper<MyForm3, ParentForm, ParentForm::Call> {
public:
    static void impl(Player& player) {
        auto fm = createForm();
        // do something
        fm.sendTo(player);
    }
};
MyForm3::send(player);

*/

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


// 表单包装器模板类
// Impl: 实现类(必须包含一个 static impl 方法)
// ParentForm: 父表单类(可选, 必须继承 FormWrapper)
// ParentCall: 父表单的回调函数(可选)
// BackPos: 返回按钮位置(可选)
template <typename Impl, typename ParentForm = void, auto ParentCall = nullptr, auto BackPos = SimpleFormExBack::Lower>
class FormWrapper {
public:
    static void send(Player& player) {
        static_assert(HasImplMethod<Impl>::value, "Impl must have a static impl method");
        Impl::impl(player);
    }

protected:
    static SimpleFormEx createForm() {
        if constexpr (std::is_same_v<ParentForm, void>) {
            return SimpleFormEx{}; // 没有父表单
        } else {
            return SimpleFormEx{
                [](Player& p) {
                    if constexpr (!std::is_same_v<ParentForm, void>) {
                        if constexpr (ParentCall != nullptr) {
                            ParentCall(p);
                        } else {
                            ParentForm::send(p);
                        }
                    }
                },
                BackPos
            };
        }
    }
};

} // namespace wrapper