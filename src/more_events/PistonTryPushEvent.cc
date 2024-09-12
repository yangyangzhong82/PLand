#include "PistonTryPushEvent.h"
#include "ll/api/event/Emitter.h"
#include "ll/api/event/EmitterBase.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/memory/Hook.h"
#include <mc/world/actor/boss/WitherBoss.h>
#include <mc/world/level/block/actor/PistonBlockActor.h>


namespace more_events {


BlockSource&    PistonTryPushEvent::getRegion() const { return mRegion; }
BlockPos const& PistonTryPushEvent::getPistonPos() const { return mPistonPos; }
BlockPos const& PistonTryPushEvent::getPushPos() const { return mPushPos; }

LL_TYPE_INSTANCE_HOOK(
    PistonPushHook,
    HookPriority::Normal,
    PistonBlockActor,
    &PistonBlockActor::_attachedBlockWalker,
    bool,
    BlockSource&    region,
    BlockPos const& curPos,
    uchar           curBranchFacing,
    uchar           pistonMoveFacing
) {
    PistonTryPushEvent ev(region, this->getPosition(), curPos);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return false;
    }

    return origin(region, curPos, curBranchFacing, pistonMoveFacing);
}


static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&);
class EventEmitter : public ll::event::Emitter<emitterFactory, PistonTryPushEvent> {
    ll::memory::HookRegistrar<PistonPushHook> hook;
};
static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&) {
    return std::make_unique<EventEmitter>();
}


} // namespace more_events