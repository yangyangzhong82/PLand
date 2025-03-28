#pragma once
#include "Global.h"
#include "pland/LandPos.h"
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

        LDNDAPI static Variable make(LandPos const& landPos);
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


} // namespace land
