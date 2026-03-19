#pragma once
#include "ll/api/Expected.h"
#include "pland/Global.h"
#include "pland/aabb/LandAABB.h"

#include <memory>

class Player;

namespace land {
class Land;
enum class LandRecycleReason : uint8_t;
class LandRegistry;
class OrdinaryLandCreateSelector;

namespace service {
class SelectionService;
class LandPriceService;


class LeasingService {
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    LeasingService(LandRegistry& registry, LandPriceService& priceService, SelectionService& selectionService);
    ~LeasingService();

    LD_DISABLE_COPY_AND_MOVE(LeasingService);

    static std::string formatDuration(long long sec, std::string_view localeCode) noexcept;

    /**
     * @brief 检查功能是否启用
     * @return 返回功能是否启用的状态
     */
    LDNDAPI bool enabled() const;


    /**
     * @brief 租赁领地
     * @param player 玩家对象引用
     * @param selector 普通领地创建选择器指针
     * @param days 租赁天数
     */
    LDNDAPI ll::Expected<std::shared_ptr<Land>>
            leaseLand(Player& player, OrdinaryLandCreateSelector* selector, int days);

    /**
     * @brief 续租领地
     * @param player 玩家对象引用
     * @param land 领地对象的共享指针
     * @param days 续租天数
     */
    LDNDAPI ll::Expected<> renewLease(Player& player, std::shared_ptr<Land> const& land, int days);

    /**
     * @brief 回收领地
     * @param land 领地对象的共享指针
     * @param reason 领地回收原因
     */
    ll::Expected<> recycleLand(std::shared_ptr<Land> const& land, LandRecycleReason reason);

    /**
     * 重新激活领地租约
     * @param player 操作玩家
     * @param land 需要重新激活的领地对象
     * @param days 租约天数
     * @param append 是否追加模式，默认为true
     *                  true: 将新天数追加到现有租约(保留原起始点)
     *                  false: 替换现有租约天数(以当前时间作为起始点)
     */
    ll::Expected<> reactivateLease(Player& player, std::shared_ptr<Land> const& land, int days, bool append = true);
};

} // namespace service
} // namespace land
