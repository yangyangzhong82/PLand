#pragma once
#include "ll/api/event/Cancellable.h"
#include "ll/api/event/Event.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"


namespace more_events {


class ExplodeEvent final : public ll::event::Cancellable<ll::event::Event> {
protected:
    BlockSource& mRegion;
    Actor*       mSourceEntity;
    Vec3 const&  mPos;
    float&       mExplosionRadius;
    float&       mMaxResistance;
    bool&        mIsFire;
    bool&        mDestroyBlocks;
    bool&        mAllowUnderWater;

public:
    constexpr explicit ExplodeEvent(
        BlockSource& region,
        Actor*       sourceEntity,
        Vec3 const&  pos,
        float&       explosionRadius,
        float&       maxResistance,
        bool&        isFire,
        bool&        destroyBlocks,
        bool&        allowUnderWater
    )
    : Cancellable(),
      mRegion(region),
      mSourceEntity(sourceEntity),
      mPos(pos),
      mExplosionRadius(explosionRadius),
      mMaxResistance(maxResistance),
      mIsFire(isFire),
      mDestroyBlocks(destroyBlocks),
      mAllowUnderWater(allowUnderWater) {}

    BlockSource& getRegion() const;
    Actor*       getSourceEntity() const;
    Vec3 const&  getPos() const;
    float&       getExplosionRadius() const;
    float&       getMaxResistance() const;
    bool&        getIsFire() const;
    bool&        getDestroyBlocks() const;
    bool&        getAllowUnderWater() const;
};


} // namespace more_events