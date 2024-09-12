#include "ExplodeEvent.h"
#include "ll/api/event/Emitter.h"
#include "ll/api/event/EmitterBase.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/memory/Hook.h"
#include <mc/world/level/Level.h>
#include <mc/world/level/block/actor/PistonBlockActor.h>


namespace more_events {


BlockSource& ExplodeEvent::getRegion() const { return mRegion; }
Actor*       ExplodeEvent::getSourceEntity() const { return mSourceEntity; }
Vec3 const&  ExplodeEvent::getPos() const { return mPos; }
float&       ExplodeEvent::getExplosionRadius() const { return mExplosionRadius; }
float&       ExplodeEvent::getMaxResistance() const { return mMaxResistance; }
bool&        ExplodeEvent::getIsFire() const { return mIsFire; }
bool&        ExplodeEvent::getDestroyBlocks() const { return mDestroyBlocks; }
bool&        ExplodeEvent::getAllowUnderWater() const { return mAllowUnderWater; }


LL_TYPE_INSTANCE_HOOK(
    ExplodeHook,
    HookPriority::Normal,
    Level,
    &Level::explode,
    bool,
    BlockSource& region,
    Actor*       source,
    Vec3 const&  pos,
    float        explosionRadius,
    bool         fire,
    bool         breaksBlocks,
    float        maxResistance,
    bool         allowUnderwater
) {
    ExplodeEvent ev(region, source, pos, explosionRadius, maxResistance, fire, breaksBlocks, allowUnderwater);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return false;
    }

    return origin(region, source, pos, explosionRadius, fire, breaksBlocks, maxResistance, allowUnderwater);
}


static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&);
class EventEmitter : public ll::event::Emitter<emitterFactory, ExplodeEvent> {
    ll::memory::HookRegistrar<ExplodeHook> hook;
};
static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&) {
    return std::make_unique<EventEmitter>();
}


} // namespace more_events