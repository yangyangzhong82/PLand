#pragma once
#include "ll/api/event/Cancellable.h"
#include "ll/api/event/Event.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/boss/WitherBoss.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"


namespace more_events {


class WitherDestroyBlockEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Level&       mLevel;
    AABB const&  mAABB;
    BlockSource& mRegion;
    WitherBoss&  mWitherBoss;

public:
    constexpr explicit WitherDestroyBlockEvent(
        Level&       level,
        AABB const&  aabb,
        BlockSource& region,
        WitherBoss&  witherBoss
    )
    : Cancellable(),
      mLevel(level),
      mAABB(aabb),
      mRegion(region),

      mWitherBoss(witherBoss) {}

    Level&       getLevel() const;
    AABB const&  getAABB() const;
    BlockSource& getRegion() const;
    WitherBoss&  getWitherBoss() const;
};


} // namespace more_events