#include "PressurePlateTriggerEvent.h"
#include "ll/api/event/Emitter.h"
#include "ll/api/event/EmitterBase.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/memory/Hook.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include <mc/world/level/block/BasePressurePlateBlock.h>


namespace more_events {

BlockSource&    PressurePlateTriggerEvent::getRegion() const { return mRegion; }
BlockPos const& PressurePlateTriggerEvent::getPos() const { return mPos; }
Actor&          PressurePlateTriggerEvent::getEntity() const { return mEntity; }


LL_TYPE_INSTANCE_HOOK(
    PressurePlateTriggerHook,
    HookPriority::Normal,
    BasePressurePlateBlock,
    "?shouldTriggerEntityInside@BasePressurePlateBlock@@UEBA_NAEAVBlockSource@@AEBVBlockPos@@AEAVActor@@@Z",
    bool,
    BlockSource&    region,
    BlockPos const& pos,
    Actor&          entity
) {
    PressurePlateTriggerEvent ev(region, pos, entity);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) {
        return false;
    }
    return origin(region, pos, entity);
}


static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&);
class EventEmitter : public ll::event::Emitter<emitterFactory, PressurePlateTriggerEvent> {
    ll::memory::HookRegistrar<PressurePlateTriggerHook> hook;
};
static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&) {
    return std::make_unique<EventEmitter>();
}


} // namespace more_events