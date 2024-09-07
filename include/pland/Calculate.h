#pragma once
#include "Global.h"
#include "pland/LandPos.h"

namespace land {

class Calculate {
public:
    double mHeight; // 高度
    double mWidth;  // 宽度
    double mDepth;  // 深度(长)
    double mSquare; // 底面积(长*宽)
    double mVolume; // 体积(长*宽*高)

    Calculate(LandPos& landPos);
    Calculate(int height, int width, int depth);

    int eval(string code);

    static int calculateDiscountPrice(int originalPrice, double discountRate); // 计算折扣后的价格
    static int calculateRefundsPrice(int originalPrice, double refundRate);    // 计算退款金额
};


} // namespace land
