#include "pland/LandDraw.h"
#include "ll/api/schedule/Task.h"
#include "ll/api/service/Bedrock.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "pland/Config.h"
#include "pland/Global.h"
#include "pland/LandPos.h"
#include "pland/PLand.h"
#include "pland/Particle.h"
#include <stop_token>
#include <thread>


namespace land {

std::unordered_map<UUIDm, std::pair<bool, std::vector<Particle>>> LandDraw::mDrawList;


void LandDraw::release() { disable(); }
void LandDraw::disable() { mDrawList.clear(); }
void LandDraw::disable(Player& player) { mDrawList.erase(player.getUuid().asString()); }

void LandDraw::enable(Player& player, bool onlyDrawCurrentLand) {
    auto& data = mDrawList[player.getUuid().asString()];
    data.first = onlyDrawCurrentLand;
}

void LandDraw::setup() {
    auto db = &PLand::getInstance();
    GlobalTickScheduler.add<ll::schedule::RepeatTask>(20_tick, [db]() {
        ll::service::getLevel()->forEachPlayer([db](Player& player) {
            if (!mDrawList.contains(player.getUuid())) {
                return true;
            }

            auto& data = mDrawList[player.getUuid()];
            data.second.clear();

            if (data.first) {
                // only draw current land
                auto land = db->getLandAt(player.getPosition(), player.getDimensionId());
                if (land) {
                    auto pp = land->getLandPos();
                    data.second.push_back(Particle{pp, land->getLandDimid(), land->is3DLand()});
                }

            } else {
                // draw all lands in range
                auto lds = db->getLandAt(player.getPosition(), Config::cfg.land.drawRange, player.getDimensionId());
                data.second.reserve(lds.size());

                for (auto& land : lds) {
                    auto pp = land->getLandPos();
                    data.second.push_back(Particle{pp, land->getLandDimid(), land->is3DLand()});
                }
            }

            return true;
        });
    });
}

} // namespace land