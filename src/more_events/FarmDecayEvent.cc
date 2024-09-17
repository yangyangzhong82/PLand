#include "FarmDecayEvent.h"
#include "ll/api/event/Emitter.h"
#include "ll/api/event/EmitterBase.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/memory/Hook.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/block/FarmBlock.h"
#include <mc/world/level/Level.h>
#include <mc/world/level/block/actor/PistonBlockActor.h>


namespace more_events {


Actor*          FarmDecayEvent::getActor() const { return mActor; }
BlockSource&    FarmDecayEvent::getBlockSource() const { return mBlockSource; }
BlockPos const& FarmDecayEvent::getPos() const { return mPos; }


LL_TYPE_INSTANCE_HOOK(
    FarmDecayHook,
    HookPriority::Normal,
    FarmBlock,
    "?transformOnFall@FarmBlock@@UEBAXAEAVBlockSource@@AEBVBlockPos@@PEAVActor@@M@Z",
    void,
    BlockSource&    region,
    BlockPos const& pos,
    Actor*          actor,
    float           fallDistance
) {
    FarmDecayEvent ev(actor, region, pos);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return;
    }

    origin(region, pos, actor, fallDistance);
}


static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&);
class EventEmitter : public ll::event::Emitter<emitterFactory, FarmDecayEvent> {
    ll::memory::HookRegistrar<FarmDecayHook> hook;
};
static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&) {
    return std::make_unique<EventEmitter>();
}


} // namespace more_events