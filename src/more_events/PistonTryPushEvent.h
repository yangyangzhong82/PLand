#pragma once
#include "ll/api/event/Cancellable.h"
#include "ll/api/event/Event.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"


namespace more_events {


class PistonTryPushEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    BlockSource&    mRegion;
    BlockPos const& mPistonPos;
    BlockPos const& mPushPos;

public:
    constexpr explicit PistonTryPushEvent(BlockSource& region, BlockPos const& pistonPos, BlockPos const& pushPos)
    : Cancellable(),
      mRegion(region),
      mPistonPos(pistonPos),
      mPushPos(pushPos) {}

    BlockSource&    getRegion() const;
    BlockPos const& getPistonPos() const;
    BlockPos const& getPushPos() const;
};


} // namespace more_events