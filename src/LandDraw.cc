#include "pland/LandDraw.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"
#include "pland/Config.h"
#include "pland/Global.h"
#include "pland/LandPos.h"
#include "pland/PLand.h"
#include "pland/Particle.h"
#include <chrono>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>


namespace land {

std::unordered_map<UUIDm, LandDraw::DrawType> LandDraw::mDrawType;

void LandDraw::disable() { mDrawType.clear(); }
void LandDraw::disable(Player& player) { mDrawType.erase(player.getUuid()); }
void LandDraw::enable(Player& player, DrawType type) { mDrawType[player.getUuid()] = type; }

void LandDraw::release() { disable(); }
void LandDraw::setup() {
    auto db = &PLand::getInstance();

    // 主线程绘制粒子
    // repeat task
    ll::coro::keepThis([db]() -> ll::coro::CoroTask<> {
        while (GlobalRepeatCoroTaskRunning) {
            co_await 25_tick;
            if (!GlobalRepeatCoroTaskRunning) co_return;
            ll::service::getLevel()->forEachPlayer([db](Player& player) {
                try {
                    auto iter = mDrawType.find(player.getUuid());
                    if (iter == mDrawType.end() || iter->second == DrawType::Disable) {
                        return true;
                    }

                    auto const& type = iter->second;
                    if (type == DrawType::CurrentLand) {
                        auto land = db->getLandAt(player.getPosition(), player.getDimensionId());
                        if (land) {
                            auto pos = land->getLandPos();
                            Particle(pos, land->getLandDimid(), land->is3DLand())
                                .draw(player, false, true, true); // 绘制当前领地
                        }

                    } else {
                        auto lds =
                            db->getLandAt(player.getPosition(), Config::cfg.land.drawRange, player.getDimensionId());

                        for (auto& land : lds) {
                            auto pp = land->getLandPos();
                            Particle{pp, land->getLandDimid(), land->is3DLand()}.draw(player, false, true, true);
                        }
                    }
                } catch (...) {
                    disable(player);
                }
                return true;
            });
        }
    }).launch(ll::thread::ServerThreadExecutor::getDefault());
}

} // namespace land