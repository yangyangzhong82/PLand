#include "pland/service/LeasingService.h"

#include "LandPriceService.h"
#include "SelectionService.h"

#include "ll/api/coro/CoroTask.h"
#include "ll/api/coro/InterruptableSleep.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/thread/ServerThreadExecutor.h"

#include "pland/PLand.h"
#include "pland/economy/EconomySystem.h"
#include "pland/economy/PriceCalculate.h"
#include "pland/events/domain/LandRecycleEvent.h"
#include "pland/events/domain/LandStateChangedEvent.h"
#include "pland/events/player/PlayerLeaseLandEvent.h"
#include "pland/events/player/PlayerMoveEvent.h"
#include "pland/events/player/PlayerRenewLandEvent.h"
#include "pland/infra/TimerScheduler.h"
#include "pland/land/Config.h"
#include "pland/land/Land.h"
#include "pland/land/LandType.h"
#include "pland/land/repo/LandRegistry.h"
#include "pland/land/validator/LandCreateValidator.h"
#include "pland/selector/land/OrdinaryLandCreateSelector.h"
#include "pland/service/LandHierarchyService.h"
#include "pland/service/ServiceLocator.h"
#include "pland/utils/FeedbackUtils.h"
#include "pland/utils/TimeUtils.h"

#include <algorithm>
#include <chrono>

namespace land::service {

struct LeasingService::Impl {
    LandRegistry&     mRegistry;
    LandPriceService& mLandPriceService;
    SelectionService& mSelectionService;

    TimerScheduler<LandID> mScheduler;

    ll::event::ListenerPtr mPlayerJoinListener{nullptr};
    ll::event::ListenerPtr mPlayerEnterLandListener{nullptr};

    std::shared_ptr<std::atomic<bool>>            mQuit{nullptr};
    std::shared_ptr<ll::coro::InterruptableSleep> mSleep{nullptr};

    explicit Impl(LandRegistry& registry, LandPriceService& priceService, SelectionService& selectionService)
    : mRegistry(registry),
      mLandPriceService(priceService),
      mSelectionService(selectionService) {}


    ll::Expected<> enqueue(std::shared_ptr<Land> const& land) {
        if (!land) {
            return ll::makeStringError("enqueue failed, land is null");
        }
        if (!land->isLeased()) {
            return ll::makeStringError("enqueue failed, land is not leased");
        }
        mScheduler.push(land->getLeaseEndAt(), land->getId());
        return {};
    }
};

LeasingService::LeasingService(
    LandRegistry&     registry,
    LandPriceService& priceService,
    SelectionService& selectionService
)
: impl(std::make_unique<Impl>(registry, priceService, selectionService)) {
    auto const& conf = ConfigProvider::getLeasingConfig();
    if (!conf.enabled) {
        return; // 租赁系统未启用，资源无需初始化
    }

    auto& bus = ll::event::EventBus::getInstance();

    impl->mQuit  = std::make_shared<std::atomic<bool>>(false);
    impl->mSleep = std::make_shared<ll::coro::InterruptableSleep>();

    if (conf.notifications.loginTip)
        impl->mPlayerJoinListener =
            bus.emplaceListener<ll::event::PlayerJoinEvent>([this](ll::event::PlayerJoinEvent& ev) {
                auto& player = ev.self();
                if (player.isSimulatedPlayer()) return;

                auto now   = time_utils::nowSeconds();
                auto lands = impl->mRegistry.getLands(player.getUuid());

                std::vector<std::shared_ptr<Land>> frozen;
                std::vector<std::shared_ptr<Land>> nearDue;

                for (auto& land : lands) {
                    if (!land->isLeased()) continue;

                    if (land->isLeaseFrozen()) {
                        frozen.push_back(land);
                        continue;
                    }

                    auto remaining = land->getLeaseEndAt() - now;
                    if (remaining
                        <= time_utils::toSeconds(ConfigProvider::getLeasingConfig().duration.renewalAdvance)) {
                        nearDue.push_back(land);
                    }
                }

                if (!frozen.empty()) {
                    feedback_utils::sendText(
                        player,
                        "您有 {} 个租赁领地已冻结，请尽快缴费续租!"_trl(player.getLocaleCode(), frozen.size())

                    );
                }
                if (!nearDue.empty()) {
                    feedback_utils::sendText(
                        player,
                        "您有 {} 个租赁领地即将到期，请及时续租!"_trl(player.getLocaleCode(), nearDue.size())
                    );
                }
            });

    if (conf.notifications.enterTip)
        impl->mPlayerEnterLandListener =
            bus.emplaceListener<event::PlayerEnterLandEvent>([this](event::PlayerEnterLandEvent& ev) {
                auto& player = ev.self();
                auto  land   = impl->mRegistry.getLand(ev.landId());
                if (!land || !land->isLeased() || !land->isOwner(player.getUuid())) {
                    return;
                }

                auto now = time_utils::nowSeconds();
                if (land->isLeaseFrozen()) {
                    auto detail = impl->mLandPriceService.calculateRenewCost(land, 0);
                    feedback_utils::sendActionBar(
                        player,
                        "[Land] 已冻结，欠费 {}"_trl(player.getLocaleCode(), detail.total)
                    );
                    return;
                }

                auto remaining = land->getLeaseEndAt() - now;
                if (remaining <= time_utils::toSeconds(ConfigProvider::getLeasingConfig().duration.renewalAdvance)) {
                    auto localeCode = player.getLocaleCode();
                    feedback_utils::sendActionBar(
                        player,
                        "[Land] 租赁即将到期：{}"_trl(localeCode, formatDuration(remaining, localeCode))
                    );
                }
            });

    // 初始化定时器
    for (auto& land : impl->mRegistry.getLands()) {
        if (land->isLeased()) {
            if (land->getLeaseState() == LeaseState::Active) {
                impl->mScheduler.push(land->getLeaseEndAt(), land->getId());
            } else if (land->getLeaseState() == LeaseState::Frozen) {
                impl->mScheduler.push(land->getLeaseEndAt() + time_utils::toSeconds(conf.freeze.days), land->getId());
            }
        }
    }

    ll::coro::keepThis([quit = impl->mQuit, sleep = impl->mSleep, this]() -> ll::coro::CoroTask<> {
        while (!quit->load()) {
            co_await sleep->sleepFor(std::chrono::seconds{5});
            if (quit->load()) {
                break;
            }

            auto now = time_utils::nowSeconds();

            auto const& conf = ConfigProvider::getLeasingConfig();

            auto freezeDayTs = time_utils::toSeconds(conf.freeze.days);

            auto dueLands = impl->mScheduler.popDueTasks(now);
            for (auto landId : dueLands) {
                auto land = impl->mRegistry.getLand(landId);
                if (!land || !land->isLeased()) continue;

                if (land->getLeaseState() == LeaseState::Active) {
                    if (land->getLeaseEndAt() <= now) { // 懒惰双检，防止玩家刚才一瞬间续费了
                        auto oldState = land->getLeaseState();
                        land->setLeaseState(LeaseState::Frozen);

                        ll::event::EventBus::getInstance().publish(
                            event::LandStateChangedEvent{land, oldState, LeaseState::Frozen}
                        );

                        // 刚进入冻结，将它彻底死亡的时间推入调度器
                        impl->mScheduler.push(now + freezeDayTs, landId);
                    }
                } else if (land->getLeaseState() == LeaseState::Frozen) {
                    auto freezeEnd = land->getLeaseEndAt() + freezeDayTs;
                    if (freezeEnd <= now) {
                        (void)recycleLand(land, event::LandRecycleReason::LeaseExpired);
                    }
                }
            }
        }
        co_return;
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

LeasingService::~LeasingService() {
    auto& bus = ll::event::EventBus::getInstance();
    bus.removeListener(impl->mPlayerJoinListener);
    bus.removeListener(impl->mPlayerEnterLandListener);
    impl->mQuit->store(true);
    impl->mSleep->interrupt(true);
}

std::string LeasingService::formatDuration(long long sec, std::string_view localeCode) noexcept {
    auto parts = time_utils::decomposeDuration(sec);
    if (parts.days > 0) {
        return "{}天{}小时{}分钟"_trl(localeCode, parts.days, parts.hours, parts.minutes);
    }
    if (parts.hours > 0) {
        return "{}小时{}分钟"_trl(localeCode, parts.hours, parts.minutes);
    }
    return "{}分钟"_trl(localeCode, parts.minutes);
}

bool LeasingService::enabled() const { return ConfigProvider::getLeasingConfig().enabled; }

ll::Expected<std::shared_ptr<Land>>
LeasingService::leaseLand(Player& player, OrdinaryLandCreateSelector* selector, int days) {
    if (!Config::ensureLeasingEnabled()) {
        return ll::makeStringError("租赁模式未启用"_trl(player.getLocaleCode()));
    }
    if (!selector) {
        return ll::makeStringError("无效选区"_trl(player.getLocaleCode()));
    }
    if (!Config::ensureLeasingDimensionAllowed(selector->getDimensionId())) {
        return ll::makeStringError("你所在的维度无法租赁领地"_trl(player.getLocaleCode()));
    }

    auto const& duration = ConfigProvider::getLeasingConfig().duration;
    if (days < duration.minPeriod || days > duration.maxPeriod) {
        return ll::makeStringError("租赁天数不合法"_trl(player.getLocaleCode()));
    }

    auto range = selector->newLandAABB();
    if (!range) {
        return ll::makeStringError("选区范围无效"_trl(player.getLocaleCode()));
    }
    range->fix();

    auto rentExp = impl->mLandPriceService.calculateDailyRent(*range, selector->getDimensionId(), selector->is3D());
    if (!rentExp) {
        return ll::makeStringError("价格表达式解析失败，请联系管理员"_trl(player.getLocaleCode()));
    }

    auto totalCost = rentExp.value() * static_cast<long long>(days);

    auto& economy = EconomySystem::getInstance();
    if (!economy->reduce(player.getUuid(), totalCost)) {
        return ll::makeStringError("您的余额不足，无法租赁"_trl(player.getLocaleCode()));
    }

    auto land = selector->newLand();
    auto now  = time_utils::nowSeconds();

    land->setOriginalBuyPrice(0);
    land->setHoldType(LandHoldType::Leased);
    land->setLeaseState(LeaseState::Active);
    land->setLeaseStartAt(now);
    land->setLeaseEndAt(now + time_utils::toSeconds(days));

    if (auto res = LandCreateValidator::validateCreateOrdinaryLand(impl->mRegistry, player, land); !res) {
        (void)economy->add(player.getUuid(), totalCost);
        if (res.error().isA<LandCreateValidator::ValidateError>()) {
            auto& error = res.error().as<LandCreateValidator::ValidateError>();
            return ll::makeStringError(error.translateError(player.getLocaleCode()));
        }
        return ll::makeStringError(res.error().message());
    }

    if (auto res = impl->mRegistry.addOrdinaryLand(land); !res) {
        (void)economy->add(player.getUuid(), totalCost);
        return ll::makeStringError(res.error().message());
    }

    impl->mSelectionService.endSelection(player);

    ll::event::EventBus::getInstance().publish(event::PlayerLeaseLandEvent{player, land, totalCost, days});

    impl->enqueue(land);
    return land;
}

ll::Expected<> LeasingService::renewLease(Player& player, std::shared_ptr<Land> const& land, int days) {
    if (!land || !land->isLeased()) {
        return ll::makeStringError("当前领地不是租赁模式"_trl(player.getLocaleCode()));
    }

    auto const& duration = ConfigProvider::getLeasingConfig().duration;
    if (days < duration.minPeriod || days > duration.maxPeriod) {
        return ll::makeStringError("续租天数不合法"_trl(player.getLocaleCode()));
    }

    if (!land->isOwner(player.getUuid()) && !impl->mRegistry.isOperator(player.getUuid())) {
        return ll::makeStringError("操作失败，您不是领地主人"_trl(player.getLocaleCode()));
    }

    auto detail = impl->mLandPriceService.calculateRenewCost(land, days);
    if (detail.total <= 0) {
        return ll::makeStringError("租金计算失败"_trl(player.getLocaleCode()));
    }

    auto& economy = EconomySystem::getInstance();
    if (!economy->reduce(player.getUuid(), detail.total)) {
        return ll::makeStringError("您的余额不足，无法续租"_trl(player.getLocaleCode()));
    }

    auto now     = time_utils::nowSeconds();
    auto endBase = land->getLeaseEndAt();
    if (endBase < now) {
        endBase = now;
    }

    auto oldState = land->getLeaseState();

    land->setLeaseEndAt(endBase + time_utils::toSeconds(days));
    land->setLeaseState(LeaseState::Active);

    if (oldState != land->getLeaseState()) {
        ll::event::EventBus::getInstance().publish(event::LandStateChangedEvent{land, oldState, land->getLeaseState()});
    }

    ll::event::EventBus::getInstance().publish(event::PlayerRenewLandEvent{player, land, detail.total, days});

    return impl->enqueue(land); // 玩家续租成功后，把新的时间推入调度器
}


ll::Expected<> LeasingService::recycleLand(std::shared_ptr<Land> const& land, event::LandRecycleReason reason) {
    if (!land) {
        return ll::makeStringError("Invalid land ptr");
    }

    auto members = land->getMembers();
    for (auto const& m : members) {
        land->removeLandMember(m);
    }

    assert(SYSTEM_ACCOUNT_UUID != mce::UUID::EMPTY());
    assert(SYSTEM_ACCOUNT_UUID.asString() == SYSTEM_ACCOUNT_UUID_STR);
    land->setOwner(SYSTEM_ACCOUNT_UUID);
    land->setLeaseState(LeaseState::Expired);
    land->setName("[欠费|系统所有] {}"_tr(land->getName()));

    ll::event::EventBus::getInstance().publish(event::LandRecycleEvent{land, reason});

    return {};
}

ll::Expected<>
LeasingService::reactivateLease(Player& player, std::shared_ptr<Land> const& land, int days, bool append) {
    if (!land || !land->isLeased()) {
        return ll::makeStringError("当前领地不是租赁模式"_trl(player.getLocaleCode()));
    }

    auto const ts = time_utils::nowSeconds();

    if (append) {
        land->setLeaseEndAt(land->getLeaseEndAt() + ts);
    } else {
        auto now = time_utils::nowSeconds();
        land->setLeaseStartAt(now);
        land->setLeaseEndAt(now + ts);
    }

    auto oldState = land->getLeaseState();
    land->setLeaseState(LeaseState::Active);
    if (oldState != land->getLeaseState()) {
        ll::event::EventBus::getInstance().publish(event::LandStateChangedEvent{land, oldState, land->getLeaseState()});
    }

    return impl->enqueue(land); // 重新加入调度系统
}


} // namespace land::service
