#pragma once
#include "ll/api/event/Cancellable.h"
#include "ll/api/event/Event.h"
#include "mc/world/actor/Actor.h"


namespace more_events {


class ActorRideEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    Actor& mPassenger;
    Actor& mRided;

public:
    constexpr explicit ActorRideEvent(Actor& passenger, Actor& rided)
    : Cancellable(),
      mPassenger(passenger),
      mRided(rided) {}

    Actor& getPassenger() const;
    Actor& getRided() const;
};


} // namespace more_events