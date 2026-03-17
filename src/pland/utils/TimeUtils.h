#pragma once
#include <chrono>

namespace land::time_utils {

/**
 * 获取当前时间的秒数（从Unix纪元开始计算）
 * @return 返回从1970年1月1日00:00:00 UTC到现在的秒数
 */
inline long long nowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/**
 * 将从1970年1月1日00:00:00 UTC开始的秒数转换为系统时钟时间点
 * @param seconds 自1970年1月1日00:00:00 UTC以来经过的秒数
 * @return 对应的系统时钟时间点
 */
inline std::chrono::system_clock::time_point toClockTime(long long seconds) {
    return std::chrono::system_clock::from_time_t(seconds);
}


inline constexpr long long HoursPerDay      = 24LL;                                            // 每天小时数
inline constexpr long long MinutesPerHour   = 60LL;                                            // 每小时分钟数
inline constexpr long long SecondsPerMinute = 60LL;                                            // 每分钟秒数
inline constexpr long long SecondsPerDay    = HoursPerDay * MinutesPerHour * SecondsPerMinute; // 每天秒数

/**
 * 将天数转换为秒数
 * @param days 要转换的天数
 * @return 返回转换后的秒数
 */
inline long long toSeconds(int days) noexcept { return static_cast<long long>(days) * SecondsPerDay; }

/**
 * 将秒数向上取整为天数
 * @param seconds 要转换的秒数
 * @return 返回转换后的天数，如果输入小于等于0则返回0
 */
inline long long ceilDays(long long seconds) noexcept {
    if (seconds <= 0) return 0;
    return (seconds + SecondsPerDay - 1) / SecondsPerDay; // 向上取整
}

/**
 * 格式化时间点为指定格式的字符串
 * @param time 系统时钟时间点
 * @param fmt_str 时间格式字符串，默认为"%Y-%m-%d %H:%M:%S"
 * @return 格式化后的时间字符串
 */
inline std::string
formatDuration(std::chrono::system_clock::time_point time, std::string_view fmt_str = "%Y-%m-%d %H:%M:%S") {
    return fmt::format(fmt::runtime("{:" + std::string(fmt_str) + "}"), time);
}


struct DurationParts {
    long long days{};    // 天数
    long long hours{};   // 小时数
    long long minutes{}; // 分钟数
    long long seconds{}; // 秒数
};
/**
 * @brief 将总秒数分解为天、小时、分钟和秒
 * @param totalSeconds 要分解的总秒数
 * @return DurationParts 包含分解后的天、小时、分钟和秒的结构体
 */
inline DurationParts decomposeDuration(long long totalSeconds) {
    using namespace std::chrono;

    totalSeconds = std::max(totalSeconds, 0LL);

    auto d = duration_cast<days>(seconds(totalSeconds));
    auto h = duration_cast<hours>(seconds(totalSeconds) - d);
    auto m = duration_cast<minutes>(seconds(totalSeconds) - d - h);
    auto s = seconds(totalSeconds) - d - h - m;

    return {d.count(), h.count(), m.count(), s.count()};
}

} // namespace land::time_utils