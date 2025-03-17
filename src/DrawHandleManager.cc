#include "pland/DrawHandleManager.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/deps/core/math/Color.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "pland/Config.h"
#include "pland/LandData.h"
#include "pland/PLand.h"
#include <memory>


namespace land {

DrawHandle::DrawHandle() : mGeometryGroup(bsci::GeometryGroup::createDefault()) {}

bsci::GeometryGroup::GeoId DrawHandle::draw(LandPos const& pos, DimensionType dim, const mce::Color& color) {
    auto result = mGeometryGroup->box(dim, AABB{pos.mMin_A, pos.mMax_B}, color);
    return result;
}

void DrawHandle::draw(LandData_sptr const& land, const mce::Color& color) {
    auto iter = mLandGeoMap.find(land->getLandID());
    if (iter != mLandGeoMap.end()) {
        return;
    }
    auto geoid = draw(land->getLandPos(), land->getLandDimid(), color);
    mLandGeoMap.emplace(land->getLandID(), std::make_pair(land, geoid));
}

void DrawHandle::remove(LandID landID) {
    auto iter = mLandGeoMap.find(landID);
    if (iter != mLandGeoMap.end()) {
        mGeometryGroup->remove(iter->second.second);
        mLandGeoMap.erase(iter);
    }
}

void DrawHandle::remove(bsci::GeometryGroup::GeoId geoid) { mGeometryGroup->remove(geoid); }

void DrawHandle::removeLands() {
    for (auto& [id, pair] : mLandGeoMap) {
        mGeometryGroup->remove(pair.second);
    }
    mLandGeoMap.clear();
}

void DrawHandle::reinit() {
    this->mLandGeoMap.clear();
    this->mGeometryGroup.reset();
    this->mGeometryGroup = bsci::GeometryGroup::createDefault();
}


DrawHandleManager::DrawHandleManager() = default;

DrawHandleManager& DrawHandleManager::getInstance() {
    static DrawHandleManager instance;
    return instance;
}

DrawHandle* DrawHandleManager::getOrCreateHandle(Player& player) {
    auto iter = mDrawHandles.find(player.getUuid());
    if (iter == mDrawHandles.end()) {
        auto handle = std::make_unique<DrawHandle>();
        iter        = mDrawHandles.emplace(player.getUuid(), std::move(handle)).first;
    }
    return iter->second.get();
}

void DrawHandleManager::removeHandle(Player& player) { mDrawHandles.erase(player.getUuid()); }

void DrawHandleManager::removeAllHandle() { mDrawHandles.clear(); }


} // namespace land
