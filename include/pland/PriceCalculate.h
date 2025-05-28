#pragma once
#include "Global.h"
#include "pland/math/LandAABB.h"
#include <unordered_map>

namespace land {


/**
 * @brief 价格计算类
 */
class PriceCalculate {
public:
    PriceCalculate() = delete;

public:
    struct Variable {
        using Impl = std::unordered_map<std::string, double>;

        Impl mImpl;

        LDNDAPI Variable();

        LDNDAPI Impl&       get();
        LDNDAPI Impl const& get() const;

        LDNDAPI Impl* operator->();

        template <typename T>
        LDNDAPI decltype(auto) operator[](T&& key);

        LDNDAPI static Variable make(LandAABB const& landPos);
        LDNDAPI static Variable make(int height, int width, int depth);
    };

public:
    /**
     * @brief 计算价格
     */
    LDNDAPI static double eval(std::string const& code, Variable const& variables);

    /**
     * @brief 计算折扣价
     */
    LDNDAPI static int calculateDiscountPrice(double originalPrice, double discountRate);

    /**
     * @brief 计算退款金额
     */
    LDNDAPI static int calculateRefundsPrice(double originalPrice, double refundRate);
};

// class PriceCalculate {
// public:
//     using Impl = std::unordered_map<std::string, double>;
//
//     enum class InternalFuncOptions {
//         None           = 0,
//         RandomNum      = 1 << 0, // random_num()
//         RandomNumRange = 1 << 1, // random_num_range(min, max)
//         Random         = RandomNum | RandomNumRange,
//         All            = Random
//     };
//
//
//     LD_DISALLOW_COPY(PriceCalculate);
//     virtual ~PriceCalculate() = default;
//
//     LDAPI explicit PriceCalculate(std::string expression, InternalFuncOptions options = InternalFuncOptions::All);
//     LDAPI explicit PriceCalculate(
//         std::string         expression,
//         Impl                variables,
//         InternalFuncOptions options = InternalFuncOptions::All
//     );
//
//     LDAPI                 PriceCalculate(PriceCalculate&&) noexcept;
//     LDAPI PriceCalculate& operator=(PriceCalculate&&) noexcept;
//
// public:
//     LDAPI PriceCalculate& addVariable(std::string name, double value);
//
//     LDNDAPI std::optional<double> getVariable(std::string const& name) const;
//
//     LDNDAPI std::string getExpression() const;
//
//     LDAPI void setExpression(std::string expression);
//
//     LDNDAPI Impl&       getVariables();
//     LDNDAPI Impl const& getVariables() const;
//
//     LDAPI void setOptions(InternalFuncOptions options);
//
//     LDNDAPI Result<double> eval() const;
//
// public:
//     template <typename T>
//     decltype(auto) operator[](T&& key) {
//         return mVariables[std::forward<T>(key)];
//     }
//
// private:
//     Impl                mVariables;
//     std::string         mExpression;
//     InternalFuncOptions mOptions;
// };
//
//
// namespace internals {
//
// LDNDAPI double random_num();
// LDNDAPI double random_num_range(double min, double max);
//
// } // namespace internals


} // namespace land
