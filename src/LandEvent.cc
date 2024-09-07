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


} // namespace land