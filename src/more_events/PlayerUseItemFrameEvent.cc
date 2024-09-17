#include "PlayerUseItemFrameEvent.h"
#include "ll/api/event/Emitter.h"
#include "ll/api/event/EmitterBase.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/memory/Hook.h"
#include "mc/world/level/block/ItemFrameBlock.h"
#include <mc/world/level/Level.h>
#include <mc/world/level/block/actor/PistonBlockActor.h>


namespace more_events {


BlockPos const& PlayerUseItemFrameEvent::getPos() const { return mPos; }
Player*         PlayerUseItemFrameEvent::getPlayer() const { return mPlayer; }


LL_TYPE_INSTANCE_HOOK(
    PlayerUseFrameHook1,
    HookPriority::Normal,
    ItemFrameBlock,
    "?use@ItemFrameBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@E@Z",
    bool,
    Player&         player,
    BlockPos const& pos,
    uchar           face
) {
    PlayerUseItemFrameEvent ev(&player, pos);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) return false;

    return origin(player, pos, face);
}

LL_TYPE_INSTANCE_HOOK(
    PlayerUseFrameHook2,
    HookPriority::Normal,
    ItemFrameBlock,
    "?attack@ItemFrameBlock@@UEBA_NPEAVPlayer@@AEBVBlockPos@@@Z",
    bool,
    Player*         player,
    BlockPos const& pos
) {
    PlayerUseItemFrameEvent ev(player, pos);
    ll::event::EventBus::getInstance().publish(ev);
    if (ev.isCancelled()) return false;

    return origin(player, pos);
}


static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&);
class EventEmitter : public ll::event::Emitter<emitterFactory, PlayerUseItemFrameEvent> {
    ll::memory::HookRegistrar<PlayerUseFrameHook1, PlayerUseFrameHook2> hook;
};
static std::unique_ptr<ll::event::EmitterBase> emitterFactory(ll::event::ListenerBase&) {
    return std::make_unique<EventEmitter>();
}


} // namespace more_events