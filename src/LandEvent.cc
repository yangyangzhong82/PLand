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


Player&    PlayerDeleteLandBeforeEvent::getPlayer() const { return mPlayer; }
LandID     PlayerDeleteLandBeforeEvent::getLandID() const { return mLandID; }
int const& PlayerDeleteLandBeforeEvent::getRefundPrice() const { return mRefundPrice; }
Player&    PlayerDeleteLandAfterEvent::getPlayer() const { return mPlayer; }
LandID     PlayerDeleteLandAfterEvent::getLandID() const { return mLandID; }


Player&      LandMemberChangeBeforeEvent::getPlayer() const { return mPlayer; }
UUIDs const& LandMemberChangeBeforeEvent::getTargetPlayer() const { return mTargetPlayer; }
LandID       LandMemberChangeBeforeEvent::getLandID() const { return mLandID; }
bool         LandMemberChangeBeforeEvent::isAdd() const { return mIsAdd; }
Player&      LandMemberChangeAfterEvent::getPlayer() const { return mPlayer; }
UUIDs const& LandMemberChangeAfterEvent::getTargetPlayer() const { return mTargetPlayer; }
LandID       LandMemberChangeAfterEvent::getLandID() const { return mLandID; }
bool         LandMemberChangeAfterEvent::isAdd() const { return mIsAdd; }


Player& LandOwnerChangeBeforeEvent::getPlayer() const { return mPlayer; }
Player& LandOwnerChangeBeforeEvent::getNewOwner() const { return mNewOwner; }
LandID  LandOwnerChangeBeforeEvent::getLandID() const { return mLandID; }
Player& LandOwnerChangeAfterEvent::getPlayer() const { return mPlayer; }
Player& LandOwnerChangeAfterEvent::getNewOwner() const { return mNewOwner; }
LandID  LandOwnerChangeAfterEvent::getLandID() const { return mLandID; }


Player&            LandRangeChangeBeforeEvent::getPlayer() const { return mPlayer; }
LandDataPtr const& LandRangeChangeBeforeEvent::getLandData() const { return mLandData; }
LandPos const&     LandRangeChangeBeforeEvent::getNewRange() const { return mNewRange; }
int const&         LandRangeChangeBeforeEvent::getNeedPay() const { return mNeedPay; }
int const&         LandRangeChangeBeforeEvent::getRefundPrice() const { return mRefundPrice; }
Player&            LandRangeChangeAfterEvent::getPlayer() const { return mPlayer; }
LandDataPtr const& LandRangeChangeAfterEvent::getLandData() const { return mLandData; }
LandPos const&     LandRangeChangeAfterEvent::getNewRange() const { return mNewRange; }
int const&         LandRangeChangeAfterEvent::getNeedPay() const { return mNeedPay; }
int const&         LandRangeChangeAfterEvent::getRefundPrice() const { return mRefundPrice; }


} // namespace land