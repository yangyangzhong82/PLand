#pragma once
#include "mc/math/Vec3.h"
#include "pland/Global.h"
#include <unordered_map>


namespace land {


class LandScheduler {
public:
    LandScheduler()                                = delete;
    LandScheduler(LandScheduler&&)                 = delete;
    LandScheduler(const LandScheduler&)            = delete;
    LandScheduler& operator=(LandScheduler&&)      = delete;
    LandScheduler& operator=(const LandScheduler&) = delete;

    LDAPI static std::unordered_map<UUIDm, LandDimid> mDimidMap;
    LDAPI static std::unordered_map<UUIDm, LandID>    mLandidMap;


    LDAPI static bool setup();   // setup the scheduler
    LDAPI static bool release(); // release the scheduler
};


} // namespace land
