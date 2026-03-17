#pragma once
#include "pland/Global.h"
#include "pland/events/LandEventMixin.h"

namespace land::event {

enum class LandRecycleReason : uint8_t {
    LeaseExpired = 0, // 租赁到期
    Inactivity   = 1, // 闲置
};

class LandRecycleEvent final : public LandEventMixin<ll::event::Event> {
    LandRecycleReason mReason;

public:
    explicit LandRecycleEvent(std::shared_ptr<Land> land, LandRecycleReason reason)
    : LandEventMixin(std::move(land)),
      mReason(reason) {}

    LDNDAPI LandRecycleReason reason() const;
};

} // namespace land::event
