#include "pland/Calculate.h"
#include "pland/LandPos.h"


#pragma warning(disable : 4702)
#include "exprtk.hpp"
#pragma warning(default : 4702)

namespace land {

Calculate::Calculate(int height, int width, int depth)
: mHeight((double)height),
  mWidth((double)width),
  mDepth((double)depth) {
    mSquare = mWidth * mDepth;
    mVolume = mHeight * mSquare;
}
Calculate::Calculate(LandPos& land) {
    mHeight = land.getHeight();
    mWidth  = land.getWidth();
    mDepth  = land.getDepth();
    mSquare = land.getSquare();
    mVolume = land.getVolume();
}

int Calculate::eval(string code) {
    // 注册变量
    exprtk::symbol_table<double> vars;
    vars.add_variable("height", mHeight);
    vars.add_variable("width", mWidth);
    vars.add_variable("depth", mDepth);
    vars.add_variable("square", mSquare);
    vars.add_variable("volume", mVolume);

    // 解析表达式
    exprtk::expression<double> expr;
    expr.register_symbol_table(vars);

    // 编译表达式
    exprtk::parser<double> parser;
    if (!parser.compile(code, expr)) {
        return 0;
    }

    return (int)expr.value(); // 计算结果
}


int Calculate::calculateDiscountPrice(int originalPrice, double discountRate) {
    // discountRate为1时表示原价，为0.9时表示打9折
    return (int)(originalPrice * discountRate);
}

int Calculate::calculateRefundsPrice(int originalPrice, double refundRate) {
    // refundRate为1时表示全额退款，为0.9时表示退还90%
    return (int)(originalPrice * refundRate);
}


} // namespace land