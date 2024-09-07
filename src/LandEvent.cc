#include "pland/LandEvent.h"
#include "mc/world/actor/player/Player.h"
#include "pland/LandData.h"


namespace land {

Player& PlayerAskCreateLandBeforeEvent::getPlayer() const { return mPlayer; }
Player& PlayerAskCreateLandAfterEvent::getPlayer() const { return mPlayer; }
bool    PlayerAskCreateLandAfterEvent::is3DLand() const { return mIs3DLand; }


Player&           PlayerBuyLandBeforeEvent::getPlayer() const { return mPlayer; }
LandSelectorData* PlayerBuyLandBeforeEvent::getLandSelectorData() const { return mLandSelectorData; }
int&              PlayerBuyLandBeforeEvent::getPrice() const { return mPrice; }
Player&           PlayerBuyLandAfterEvent::getPlayer() const { return mPlayer; }
LandDataPtr       PlayerBuyLandAfterEvent::getLandData() const { return mLandData; }


Player& PlayerEnterLandEvent::getPlayer() const { return mPlayer; }
LandID  PlayerEnterLandEvent::getLandID() const { return mLandID; }
Player& PlayerLeaveLandEvent::getPlayer() const { return mPlayer; }
LandID  PlayerLeaveLandEvent::getLandID() const { return mLandID; }


} // namespace land