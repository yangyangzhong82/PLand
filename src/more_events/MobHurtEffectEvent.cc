#include "MobHurtEffectEvent.h"
#include "ll/api/event/Emitter.h"
#include "ll/api/event/EmitterBase.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/memory/Hook.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDamageSource.h"
#include "mc/world/actor/Mob.h"
#include <mc/world/level/Level.h>
#include <mc/world/level/block/actor/PistonBlockActor.h>


namespace more_events {


ActorDamageSource const& MobHurtEffectEvent::getSource() const { return mSource; }
Actor&                   MobHurtEffectEvent::getSelf() const { return mSelf; }
float&                   MobHurtEffectEvent::getDamage() const { return mDamage; }

LL_TYPE_INSTANCE_HOOK(
    MobHurtEffectHook,
    HookPriority::Normal,
    Mob,
    &Mob::getDamageAfterResistanceEffect,
    float,
    ActorDamageSource const& source,
    float                    damage
) {
    if (source.getCause() == ActorDamageCause::Magic || source.getCause() == ActorDamageCause::Wither) {
        // Actor* damageSource = nullptr;
        // if (source.isEntitySource()) {
        //     if (source.isChildEntitySource()) {
        //         damageSource = ll::service::getLevel()->fetchEntity(source.getEntityUniqueID());
        //     } else {
        //         damageSource = ll::service::getLevel()->fetchEntity(source.getDamagingEntityUniqueID());
        //     }
        // }
        MobHurtEffectEvent ev(source, *this, damage);
        ll::event::EventBus::getInstance().publish(ev);
        if (ev.isCancelled()) {
            return 0.0f;
        }
    }

    return origin(source, damage);
}


static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&);
class EventEmitter : public ll::event::Emitter<emitterFactory, MobHurtEffectEvent> {
    ll::memory::HookRegistrar<MobHurtEffectHook> hook;
};
static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&) {
    return std::make_unique<EventEmitter>();
}


} // namespace more_events