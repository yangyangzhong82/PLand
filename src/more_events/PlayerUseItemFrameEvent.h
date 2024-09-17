#pragma once
#include "ll/api/event/Cancellable.h"
#include "ll/api/event/Event.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"


namespace more_events {


class PlayerUseItemFrameEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    BlockPos const& mPos;
    Player*         mPlayer;


public:
    constexpr explicit PlayerUseItemFrameEvent(Player* player, BlockPos const& pos)
    : Cancellable(),
      mPos(pos),
      mPlayer(player) {}

    BlockPos const& getPos() const;
    Player*         getPlayer() const;
};


} // namespace more_events