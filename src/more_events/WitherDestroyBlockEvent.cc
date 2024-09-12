#include "WitherDestroyBlockEvent.h"
#include "ll/api/event/Emitter.h"
#include "ll/api/event/EmitterBase.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/memory/Hook.h"
#include <mc/world/actor/boss/WitherBoss.h>


namespace more_events {


Level&       WitherDestroyBlockEvent::getLevel() const { return mLevel; }
AABB const&  WitherDestroyBlockEvent::getAABB() const { return mAABB; }
BlockSource& WitherDestroyBlockEvent::getRegion() const { return mRegion; }
WitherBoss&  WitherDestroyBlockEvent::getWitherBoss() const { return mWitherBoss; }

LL_TYPE_INSTANCE_HOOK(
    WitherDestroyHook,
    HookPriority::Normal,
    WitherBoss,
    &WitherBoss::_destroyBlocks,
    void,
    Level&                       level,
    AABB const&                  bb,
    BlockSource&                 region,
    int                          range,
    WitherBoss::WitherAttackType type
) {
    WitherDestroyBlockEvent ev(level, bb, region, *this);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    origin(level, bb, region, range, type);
}


static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&);
class EventEmitter : public ll::event::Emitter<emitterFactory, WitherDestroyBlockEvent> {
    ll::memory::HookRegistrar<WitherDestroyHook> hook;
};
static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&) {
    return std::make_unique<EventEmitter>();
}


} // namespace more_events