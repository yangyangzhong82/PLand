#include "LandPriceService.h"

#include "LandHierarchyService.h"
#include "pland/economy/PriceCalculate.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/utils/TimeUtils.h"

namespace land {
namespace service {

struct LandPriceService::Impl {
    LandHierarchyService& mHierarchyService;
};

LandPriceService::LandPriceService(LandHierarchyService& service) : impl(std::make_unique<Impl>(service)) {}
LandPriceService::~LandPriceService() = default;

int64_t LandPriceService::calculatePriceRecursively(
    std::shared_ptr<Land>                                              land,
    std::function<bool(std::shared_ptr<Land> const&, int64_t&)> const& func
) const {
    int64_t result = 0;

    auto iter = impl->mHierarchyService.getDescendants(land);
    for (auto i : iter) {
        if (!func) {
            result += i->getOriginalBuyPrice();
        } else {
            if (!func(i, result)) {
                break;
            }
        }
    }
    return result;
}
ll::Expected<LandPriceService::PriceResult>
LandPriceService::getOrdinaryLandPrice(LandAABB const& range, int dimId, bool is3D) const {
    return _getLandPrice(range, dimId, Config::getLandPriceCalculateFormula(is3D));
}
ll::Expected<LandPriceService::PriceResult> LandPriceService::getSubLandPrice(LandAABB const& range, int dimId) const {
    return _getLandPrice(range, dimId, Config::getSubLandPriceCalculateFormula());
}
int64_t LandPriceService::getRefundAmount(std::shared_ptr<Land> const& land) const {
    auto const& conf = ConfigProvider::getBoughtConfig();
    return PriceCalculate::calculateRefundsPrice(land->getOriginalBuyPrice(), conf.refundRate);
}
int64_t LandPriceService::getRefundAmountRecursively(std::shared_ptr<Land> const& land) const {
    auto const& conf = ConfigProvider::getBoughtConfig();
    return calculatePriceRecursively(land, [&conf](std::shared_ptr<Land> const& land, int64_t& price) {
        price += PriceCalculate::calculateRefundsPrice(land->getOriginalBuyPrice(), conf.refundRate);
        return true;
    });
}

ll::Expected<long long> LandPriceService::calculateDailyRent(LandAABB const& range, int dimId, bool is3D) const {
    if (!Config::ensureEconomySystemEnabled()) {
        return ll::makeStringError("Economy system is not enabled");
    }

    auto& formula = ConfigProvider::getLeasingFormula(is3D);

    auto variable = PriceCalculate::Variable::make(range, dimId);
    auto expected = PriceCalculate::eval(formula, variable);
    if (!expected) return ll::makeStringError(expected.error().message());

    auto dailyRent = expected.value();
    if (auto multipliers = Config::getLandDimensionMultipliers(dimId)) {
        dailyRent *= *multipliers;
    }

    return std::llround(dailyRent);
}

ll::Expected<long long>
LandPriceService::calculateLeaseCost(LandAABB const& range, int dimId, bool is3D, int days) const {
    auto daily = calculateDailyRent(range, dimId, is3D);
    if (!daily) return daily.error();
    return daily.value() * static_cast<long long>(days);
}

LandPriceService::LeaseCostDetail
LandPriceService::calculateRenewCost(std::shared_ptr<Land> const& land, int days) const {
    LeaseCostDetail detail{};
    if (!land || !land->isLeased()) return detail;

    auto dailyRent = 0;
    if (auto calc = calculateDailyRent(land->getAABB(), land->getDimensionId(), land->is3D())) {
        dailyRent = calc.value();
    }

    auto now         = time_utils::nowSeconds();
    auto overdueDays = time_utils::ceilDays(now - land->getLeaseEndAt());
    if (overdueDays < 0) overdueDays = 0;

    detail.dailyRent   = dailyRent;
    detail.overdueDays = overdueDays;
    detail.baseAmount  = dailyRent * (overdueDays + std::max(0, days));

    auto rate      = ConfigProvider::getLeasingConfig().freeze.penaltyRatePerDay;
    detail.penalty = static_cast<long long>(std::ceil(dailyRent * overdueDays * rate));
    detail.total   = detail.baseAmount + detail.penalty;
    return detail;
}


ll::Expected<LandPriceService::PriceResult>
LandPriceService::_getLandPrice(LandAABB const& range, int dimId, std::string const& calculateFormula) const {
    if (!Config::ensureEconomySystemEnabled()) {
        return ll::makeStringError("Economy system is not enabled");
    }
    auto variable = PriceCalculate::Variable::make(range, dimId);
    auto expected = PriceCalculate::eval(calculateFormula, variable);
    if (!expected) return ll::makeStringError(expected.error().message());

    auto originalPrice = expected.value();
    auto multipliers   = Config::getLandDimensionMultipliers(dimId);
    if (multipliers) {
        originalPrice *= *multipliers;
    }
    auto discountedPrice = PriceCalculate::calculateDiscountPrice(originalPrice, ConfigProvider::getDiscountRate());
    return PriceResult{static_cast<long long>(originalPrice), discountedPrice, multipliers};
}


} // namespace service
} // namespace land