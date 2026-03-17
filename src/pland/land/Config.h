#pragma once
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"
#include "pland/drawer/DrawerType.h"
#include "pland/economy/EconomySystem.h"

#include <unordered_set>
#include <vector>


namespace land {


// 禁止创建领地的范围结构体
struct ForbiddenRange {
    LandAABB aabb;        // 领地坐标范围
    int      dimensionId; // 维度ID
};

struct Config {
    inline static constexpr int SchemaVersion = 33;

    int version{SchemaVersion};

    EconomyConfig economy;

    struct {
        bool   landTp{true};             // 领地传送
        int    maxLand{20};              // 最大领地数量
        int    minSpacing{16};           // 最小领地间距
        bool   minSpacingIncludeY{true}; // 最小领地间距是否包含Y轴
        double refundRate{0.9};          // 退款率(0.0~1.0，1.0为全额退款，0.9为退还90%)
        double discountRate{1};          // 折扣率(0.0~1.0，1.0为原价，0.9为打9折)

        bool setupDrawCommand{false}; // 注册领地绘制命令
        int  drawRange{64};           // 绘制 x 范围内的领地

        DrawerType drawHandleBackend{DrawerType::DebugShape}; // 领地绘制后端

        struct {
            bool        enabled{false};                              // 是否启用
            int         maxNested{5};                                // 最大嵌套层数(默认5，最大16)
            int         minSpacing{8};                               // 子领地之间的最小间距
            bool        minSpacingIncludeY{true};                    // 子领地之间的最小间距是否包含Y轴
            int         maxSubLand{6};                               // 每个领地的最大子领地数量
            std::string calculate{"(square * 8 + height * 20) * 0"}; // 价格公式
        } subLand;

        struct {
            bool enterTip{true};           // 进入领地提示
            bool bottomContinuedTip{true}; // 底部持续提示
            int  bottomTipFrequency{1};    // 底部提示频率(s)
        } tip;

        // 租赁配置
        struct {
            bool                   enabled{false};           // 是否启用租赁功能
            std::vector<LandDimid> allowDimensions{0, 1, 2}; // 允许租赁的土地维度列表

            // 租金计算相关配置
            struct {
                std::string threeDimensionl{"(square * 2 + height * 5)"}; // 三维空间的租金计算公式
                std::string twoDimensionl{"(square * 8)"};                // 二维空间的租金计算公式
            } calculate;

            // 租期相关配置
            struct {
                int minPeriod{7};      // 最小租期（天）
                int maxPeriod{30};     // 最大租期（天）
                int renewalAdvance{3}; // 续约提前天数
            } duration;

            // 冻结相关配置
            struct {
                int    duration{7};             // 冻结持续时间（天）
                double penaltyRatePerDay{0.05}; // 每日罚金比例（5%）
            } freeze;

            // 通知相关配置
            struct {
                bool loginTip{true}; // 登录时是否显示提示
                bool enterTip{true}; // 进入土地时是否显示提示
            } notifications;
        } leasing; // 租赁配置实例

        // 购买配置
        struct {
            struct {
                bool        enabled{true};
                std::string calculate{"square * 8 + height * 20"}; // 计算公式
            } threeDimensionl;

            struct {
                bool        enabled{true};
                std::string calculate{"square * 25"}; // 计算公式
            } twoDimensionl;

            struct {
                int min{4};       // 最小领地范围
                int max{60000};   // 最大领地范围
                int minHeight{1}; // 最小领地高度
            } squareRange;

            std::vector<LandDimid>        allowDimensions{0, 1, 2};   // 允许的领地维度
            std::vector<ForbiddenRange>   forbiddenRanges;            // 禁止创建领地的区域
            std::map<std::string, double> dimensionPriceCoefficients; // 维度价格系数，例如维度id的1 是1.2倍 2是1.5倍
        } bought;

        struct {
            /* 领地名称 */
            struct {
                size_t minLen{1};           // 最小长度
                size_t maxLen{32};          // 最大长度
                bool   allowNewline{false}; // 是否允许换行
            } name;
        } textRules; // 文本规则
    } land;

    struct {
        std::string tool{"minecraft:stick"}; // 工具
        std::string alias{"木棍"};           // 别名
    } selector;

    struct {
        bool telemetry{true}; // 遥测（匿名数据统计）
        bool devTools{false}; // 开发工具
    } internal;


    // Functions
    LDAPI static Config cfg;

    inline static constexpr std::string_view FileName = "Config.json";

    LDAPI static bool tryLoad();
    LDAPI static bool trySave();

    LDAPI static bool ensureDimensionAllowed(int dimensionId);
    LDAPI static bool ensureSubLandFeatureEnabled();
    LDAPI static bool ensureOrdinaryLandEnabled(bool is3D);
    LDAPI static bool ensureLeasingEnabled();
    LDAPI static bool ensureLeasingDimensionAllowed(int dimensionId);

    LDAPI static bool ensureEconomySystemEnabled();

    LDAPI static std::optional<double> getLandDimensionMultipliers(LandDimid dimid);

    LDAPI static std::string const& getLandPriceCalculateFormula(bool is3D); // 获取价格计算公式

    LDAPI static std::string const& getSubLandPriceCalculateFormula(); // 获取子领地价格计算公式
};


} // namespace land
