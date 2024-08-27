#include "pland/LandData.h"

namespace land {

bool LandData::isRadiusInLand(BlockPos const& pos, int radius) const {
    BlockPos minPos(pos.x - radius, mIs3DLand ? pos.y - radius : mPos.mMin.y, pos.z - radius);
    BlockPos maxPos(pos.x + radius, mIs3DLand ? pos.y + radius : mPos.mMax.y, pos.z + radius);
    return isAABBInLand(minPos, maxPos);
}

bool LandData::isAABBInLand(BlockPos const& pos1, BlockPos const& pos2) const {
    return LandPos::isCollision(
        mPos,
        LandPos{
            PosBase{pos1.x, pos1.y, pos1.z},
            PosBase{pos2.x, pos2.y, pos2.z}
    }
    );
}


} // namespace land