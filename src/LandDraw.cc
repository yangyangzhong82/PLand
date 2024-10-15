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
#include <chrono>
#include <stop_token>
#include <thread>


namespace land {

std::unordered_map<UUIDm, std::pair<bool, std::vector<Particle>>> LandDraw::mDrawList;
std::jthread                                                      LandDraw::mThread;


void LandDraw::disable() { mDrawList.clear(); }
void LandDraw::disable(Player& player) { mDrawList.erase(player.getUuid()); }

void LandDraw::enable(Player& player, bool onlyDrawCurrentLand) {
    auto& data = mDrawList[player.getUuid()];
    data.first = onlyDrawCurrentLand;
}


void LandDraw::release() {
    disable();
    mThread.request_stop();
}
void LandDraw::setup() {
    auto db = &PLand::getInstance();

    // 子线程查询玩家附近的领地
    mThread = std::jthread([db](std::stop_token st) {
        while (!st.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 0.5s
            try {
                auto level = ll::service::getLevel();
                if (!level) {
                    continue;
                }

                for (auto& [uuid, data] : mDrawList) {
                    auto player = level->getPlayer(uuid);
                    if (!player) {
                        mDrawList.erase(uuid); // 玩家下线
                        continue;
                    }

                    data.second.clear();
                    if (data.first) {
                        // only draw current land
                        auto land = db->getLandAt(player->getPosition(), player->getDimensionId());
                        if (land) {
                            auto pp = land->getLandPos();
                            data.second.push_back(Particle{pp, land->getLandDimid(), land->is3DLand()});
                        }

                    } else {
                        // draw all lands in range
                        auto lds =
                            db->getLandAt(player->getPosition(), Config::cfg.land.drawRange, player->getDimensionId());
                        data.second.reserve(lds.size());

                        for (auto& land : lds) {
                            auto pp = land->getLandPos();
                            data.second.push_back(Particle{pp, land->getLandDimid(), land->is3DLand()});
                        }
                    }
                }
            } catch (...) {}
        }
    });

    // 主线程绘制粒子
    GlobalTickScheduler.add<ll::schedule::RepeatTask>(20_tick, []() {
        ll::service::getLevel()->forEachPlayer([](Player& player) {
            if (!mDrawList.contains(player.getUuid())) {
                return true;
            }

            auto& data = mDrawList[player.getUuid()];
            if (data.second.empty()) {
                return true;
            }
            for (auto& particle : data.second) {
                particle.draw(player);
            }

            return true;
        });
    });
}

} // namespace land