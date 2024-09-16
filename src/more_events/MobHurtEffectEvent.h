#pragma once
#include "ll/api/event/Cancellable.h"
#include "ll/api/event/Event.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/BlockSource.h"


namespace more_events {


class MobHurtEffectEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    ActorDamageSource const& mSource;
    Actor&                   mSelf;
    float&                   mDamage;

public:
    constexpr explicit MobHurtEffectEvent(ActorDamageSource const& source, Actor& self, float& damage)
    : Cancellable(),
      mSource(source),
      mSelf(self),
      mDamage(damage) {}

    ActorDamageSource const& getSource() const;
    Actor&                   getSelf() const;
    float&                   getDamage() const;
};


} // namespace more_events