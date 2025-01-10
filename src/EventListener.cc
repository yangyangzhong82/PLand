#include "pland/EventListener.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/ListenerBase.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/network/packet/UpdateBlockPacket.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/material/Material.h"
#include "mc/world/phys/AABB.h"
#include "mc/world/phys/HitResult.h"
#include "mc\world\level\block\components\BlockLiquidDetectionComponent.h"
#include "mc\world\level\chunk\SubChunk.h"
#include "mod/MyMod.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/PLand.h"
#include "pland/utils/MC.h"
#include <functional>
#include <optional>


#include "ll/api/event/entity/ActorHurtEvent.h"
#include "ll/api/event/player/PlayerAttackEvent.h"
#include "ll/api/event/player/PlayerDestroyBlockEvent.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/event/player/PlayerPickUpItemEvent.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "ll/api/event/player/PlayerUseItemEvent.h"
#include "ll/api/event/world/FireSpreadEvent.h"


#include "ila/event/minecraft/actor/ActorRideEvent.h"
#include "ila/event/minecraft/actor/ActorTriggerPressurePlateEvent.h"
#include "ila/event/minecraft/actor/ArmorStandSwapItemEvent.h"
#include "ila/event/minecraft/actor/MobHurtEffectEvent.h"
#include "ila/event/minecraft/actor/ProjectileCreateEvent.h"
// #include "ila/event/minecraft/level/SculkCatalystAbsorbExperienceEvent.h"
#include "ila/event/minecraft/player/PlayerAttackBlockEvent.h"
#include "ila/event/minecraft/player/PlayerDropItemEvent.h"
#include "ila/event/minecraft/player/PlayerOperatedItemFrameEvent.h"
#include "ila/event/minecraft/world/ExplosionEvent.h"
#include "ila/event/minecraft/world/FarmDecayEvent.h"
#include "ila/event/minecraft/world/LiquidTryFlowEvent.h"
#include "ila/event/minecraft/world/MossGrowthEvent.h"
#include "ila/event/minecraft/world/PistonPushEvent.h"
#include "ila/event/minecraft/world/RedstoneUpdateEvent.h"
#include "ila/event/minecraft/world/SculkBlockGrowthEvent.h"
#include "ila/event/minecraft/world/SculkSpreadEvent.h"
#include "ila/event/minecraft/world/WitherDestroyEvent.h"


ll::event::ListenerPtr mPlayerJoinEvent;           // 玩家加入服务器
ll::event::ListenerPtr mActorHurtEvent;            // 实体受伤
ll::event::ListenerPtr mPlayerDestroyBlockEvent;   // 玩家尝试破坏方块
ll::event::ListenerPtr mPlayerPlaceingBlockEvent;  // 玩家尝试放置方块
ll::event::ListenerPtr mPlayerUseItemOnEvent;      // 玩家对方块使用物品（点击右键）
ll::event::ListenerPtr mFireSpreadEvent;           // 火焰蔓延
ll::event::ListenerPtr mPlayerAttackEntityEvent;   // 玩家攻击实体
ll::event::ListenerPtr mPlayerPickUpItemEvent;     // 玩家捡起物品
ll::event::ListenerPtr mPlayerInteractBlockEvent;  // 方块接受玩家互动
ll::event::ListenerPtr mPlayerUseItemEvent;        // 玩家使用物品
ll::event::ListenerPtr mArmorStandSwapItemEvent;   // 玩家交换盔甲架物品 (iListenAttentively)
ll::event::ListenerPtr mPlayerAttackBlockEvent;    // 玩家攻击方块 (iListenAttentively)
ll::event::ListenerPtr mPlayerDropItemEvent;       // 玩家丢弃物品 (iListenAttentively)
ll::event::ListenerPtr mActorRideEvent;            // 实体骑乘 (iListenAttentively)
ll::event::ListenerPtr mExplodeEvent;              // 爆炸 (iListenAttentively)
ll::event::ListenerPtr mFarmDecayEvent;            // 农田退化 (iListenAttentively)
ll::event::ListenerPtr mMobHurtEffectEvent;        // 实体受伤效果 (iListenAttentively)
ll::event::ListenerPtr mPistonTryPushEvent;        // 活塞尝试推动方块 (iListenAttentively)
ll::event::ListenerPtr mPlayerUseItemFrameEvent;   // 玩家使用物品展示框 (iListenAttentively)
ll::event::ListenerPtr mPressurePlateTriggerEvent; // 压力板触发 (iListenAttentively)
ll::event::ListenerPtr mProjectileSpawnEvent;      // 投掷物生成 (iListenAttentively)
ll::event::ListenerPtr mRedstoneUpdateEvent;       // 红石更新 (iListenAttentively)
ll::event::ListenerPtr mWitherDestroyBlockEvent;   // 凋零破坏方块 (iListenAttentively)
ll::event::ListenerPtr mMossFertilizerEvent;       // 苔藓施肥 (iListenAttentively)
ll::event::ListenerPtr mLiquidFlowEvent;           // 流体流动 (iListenAttentively)
// ll::event::ListenerPtr mSculkCatalystAbsorbExperienceEvent; // 幽匿催发体吸收经验 (iListenAttentively)
ll::event::ListenerPtr mSculkBlockGrowthEvent; // 幽匿尖啸体生成 (iListenAttentively)
ll::event::ListenerPtr mSculkSpreadEvent;      // 幽匿蔓延 (iListenAttentively)

namespace land {
inline bool PreCheck(LandData_sptr ptr, UUIDs uuid = "", bool ignoreOperator = false) {
    if (!ptr) {
        return true; // 此位置没有领地
    } else if (!ignoreOperator && PLand::getInstance().isOperator(uuid)) {
        return true; // 是管理员
    } else if (ptr->getPermType(uuid) != LandPermType::Guest) {
        return true; // 有权限 (主人/成员)
    }
    return false;
}


bool EventListener::setup() {
    auto* db     = &PLand::getInstance();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &my_mod::MyMod::getInstance().getSelf().getLogger();

    // Minecraft events (ll)
    mPlayerJoinEvent = bus->emplaceListener<ll::event::PlayerJoinEvent>([db, logger](ll::event::PlayerJoinEvent& ev) {
        if (ev.self().isSimulatedPlayer()) return;
        if (!db->hasPlayerSettings(ev.self().getUuid().asString())) {
            db->setPlayerSettings(ev.self().getUuid().asString(), PlayerSettings{}); // 新玩家
        }

        auto lands = db->getLands(ev.self().getXuid()); // xuid 查询
        if (!lands.empty()) {
            logger->info("Update land owner data from xuid to uuid for player {}", ev.self().getRealName());
            auto uuid = ev.self().getUuid().asString();
            for (auto& land : lands) {
                if (land->mIsConvertedLand && land->mOwnerDataIsXUID) {
                    land->mLandOwner       = uuid; // xuid -> uuid
                    land->mOwnerDataIsXUID = false;
                }
            }
        }
    });

    mActorHurtEvent = bus->emplaceListener<ll::event::ActorHurtEvent>([db, logger](ll::event::ActorHurtEvent& ev) {
        logger->debug("[ActorHurt] mob: {}", ev.self().getTypeName());
        auto& self = ev.self();

        auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
        if (PreCheck(land)) return true; // land not found
        if (land) {
            auto const& et  = self.getTypeName();
            auto const& tab = land->getLandPermTableConst();
            if (tab.allowAttackPlayer && self.isPlayer()) return true;
            if (tab.allowAttackAnimal && AnimalEntityMap.contains(et)) return true;
            if (tab.allowAttackMob && !AnimalEntityMap.contains(et)) return true;
        }

        if (self.isPlayer()) {
            auto const pl = self.getWeakEntity().tryUnwrap<Player>();
            if (pl.has_value()) {
                if (PreCheck(land, pl->getUuid().asString())) return true;
            }
        }

        ev.cancel();
        return true;
    });

    mPlayerDestroyBlockEvent =
        bus->emplaceListener<ll::event::PlayerDestroyBlockEvent>([db, logger](ll::event::PlayerDestroyBlockEvent& ev) {
            auto& player   = ev.self();
            auto& blockPos = ev.pos();

            logger->debug("[DestroyBlock] {}", blockPos.toString());

            auto land = db->getLandAt(blockPos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            auto& tab = land->getLandPermTableConst();
            if (tab.allowDestroy) {
                return true;
            }

            ev.cancel();
            return true;
        });

    mPlayerPlaceingBlockEvent =
        bus->emplaceListener<ll::event::PlayerPlacingBlockEvent>([db, logger](ll::event::PlayerPlacingBlockEvent& ev) {
            auto&       player   = ev.self();
            auto const& blockPos = mc::face2Pos(ev.pos(), ev.face()); // 计算实际放置位置

            logger->debug("[PlaceingBlock] {}", blockPos.toString());

            auto land = db->getLandAt(blockPos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            auto& tab = land->getLandPermTableConst();
            if (tab.allowPlace) {
                return true;
            }

            ev.cancel();
            return true;
        });

    mPlayerUseItemOnEvent =
        bus->emplaceListener<ll::event::PlayerInteractBlockEvent>([db,
                                                                   logger](ll::event::PlayerInteractBlockEvent& ev) {
            auto& player = ev.self();
            auto& vec3   = ev.clickPos();
            auto& block  = ev.block()->getTypeName();
            auto  item   = ev.item().getTypeName();

            logger->debug(
                "[UseItemOn] Pos: {}, Item: {}, Block: {}",
                vec3.toString(),
                ev.item().getTypeName(),
                ev.block()->getTypeName()
            );

            auto land = db->getLandAt(vec3, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            if (!UseItemOnMap.contains(item) && !UseItemOnMap.contains(block)) {
                return true;
            }

            // clang-format off
            auto const& tab = land->getLandPermTableConst();
            if (item.ends_with( "bucket") && tab.useBucket) return true;    // 各种桶
            if (item.ends_with( "axe") && tab.allowAxePeeled) return true; // 斧头给木头去皮
            if (item.ends_with( "hoe") && tab.useHoe) return true;         // 锄头耕地
            if (item.ends_with( "_shovel") && tab.useShovel) return true;  // 锹铲除草径
            if (item == "minecraft:skull" && tab.allowPlace) return true;           // 放置头颅
            if (item == "minecraft:banner" && tab.allowPlace) return true;          // 放置旗帜
            if (item == "minecraft:glow_ink_sac" && tab.allowPlace) return true;    // 发光墨囊给木牌上色
            if (item == "minecraft:end_crystal" && tab.allowPlace) return true;     // 末地水晶
            if (item == "minecraft:ender_eye" && tab.allowPlace) return true;       // 放置末影之眼
            if (item == "minecraft:flint_and_steel" && tab.useFiregen) return true; // 使用打火石
            if (item == "minecraft:bone_meal" && tab.useBoneMeal) return true;      // 使用骨粉
            if (item == "minecraft:minecart"&& tab.allowPlace) return true; // 放置矿车
            if (item == "minecraft:armor_stand"&& tab.allowPlace) return true; // 放置矿车

            if (block.ends_with( "button") && tab.useButton) return true;       // 各种按钮
            if (block.ends_with( "_door") && tab.useDoor) return true;            // 各种门
            if (block.ends_with( "fence_gate") && tab.useFenceGate) return true;  // 各种栏栅门
            if (block.ends_with( "trapdoor") && tab.useTrapdoor) return true;     // 各种活板门
            if (block.ends_with( "_sign") && tab.editSign) return true; // 编辑告示牌
            if (block.ends_with("shulker_box") && tab.useShulkerBox) return true;  // 潜影盒
            if (block == "minecraft:dragon_egg" && tab.allowAttackDragonEgg) return true; // 右键龙蛋
            if (block == "minecraft:bed" && tab.useBed) return true;                      // 床
            if ((block == "minecraft:chest" || block == "minecraft:trapped_chest") && tab.allowOpenChest) return true; // 箱子&陷阱箱
            if (block == "minecraft:crafting_table" && tab.useCraftingTable) return true; // 工作台
            if ((block == "minecraft:campfire" || block == "minecraft:soul_campfire") && tab.useCampfire) return true; // 营火（烧烤）
            if (block == "minecraft:composter" && tab.useComposter) return true; // 堆肥桶（放置肥料）
            if (block == "minecraft:noteblock" && tab.useNoteBlock) return true; // 音符盒（调音）
            if (block == "minecraft:jukebox" && tab.useJukebox) return true;     // 唱片机（放置/取出唱片）
            if (block == "minecraft:bell" && tab.useBell) return true;           // 钟（敲钟）
            if ((block == "minecraft:daylight_detector_inverted" || block == "minecraft:daylight_detector") && tab.useDaylightDetector) return true; // 光线传感器（切换日夜模式）
            if (block == "minecraft:lectern" && tab.useLectern) return true;                // 讲台
            if (block == "minecraft:cauldron" && tab.useCauldron) return true;              // 炼药锅
            if (block == "minecraft:lever" && tab.useLever) return true;                    // 拉杆
            if (block == "minecraft:respawn_anchor" && tab.useRespawnAnchor) return true;   // 重生锚（充能）
            if (block == "minecraft:flower_pot" && tab.editFlowerPot) return true;          // 花盆
            // clang-format on

            ev.cancel();
            return true;
        });

    mFireSpreadEvent = bus->emplaceListener<ll::event::FireSpreadEvent>([db](ll::event::FireSpreadEvent& ev) {
        auto& pos = ev.pos();

        auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
        if (PreCheck(land)) {
            return true;
        }

        if (land->getLandPermTableConst().allowFireSpread) {
            return true;
        }

        ev.cancel();
        return true;
    });

    mPlayerAttackEntityEvent =
        bus->emplaceListener<ll::event::PlayerAttackEvent>([db, logger](ll::event::PlayerAttackEvent& ev) {
            auto& player = ev.self();
            auto& pos    = ev.target().getPosition();

            logger->debug("[AttackEntity] Entity: {}, Pos: {}", ev.target().getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            auto const& et  = ev.target().getTypeName();
            auto const& tab = land->getLandPermTableConst();
            if (et == "minecraft:ender_crystal" && tab.allowAttackEnderCrystal) return true; // 末影水晶
            if (et == "minecraft:armor_stand" && tab.allowDestroyArmorStand) return true;    // 盔甲架
            if (tab.allowAttackPlayer && ev.target().isPlayer()) return true;                // 玩家
            if (tab.allowAttackAnimal && AnimalEntityMap.contains(et)) return true;          // 动物
            if (tab.allowAttackMob && !AnimalEntityMap.contains(et)) return true;            // 怪物

            ev.cancel();
            return true;
        });

    mPlayerPickUpItemEvent =
        bus->emplaceListener<ll::event::PlayerPickUpItemEvent>([db, logger](ll::event::PlayerPickUpItemEvent& ev) {
            auto& player = ev.self();
            auto& pos    = ev.itemActor().getPosition();

            logger->debug("[PickUpItem] Item: {}, Pos: {}", ev.itemActor().getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            if (land->getLandPermTableConst().allowPickupItem) return true;

            ev.cancel();
            return true;
        });

    mPlayerInteractBlockEvent =
        bus->emplaceListener<ll::event::PlayerInteractBlockEvent>([db,
                                                                   logger](ll::event::PlayerInteractBlockEvent& ev) {
            auto& player = ev.self();
            auto& pos    = ev.blockPos(); // 交互的方块位置
            auto& block  = ev.block()->getTypeName();

            logger->debug("[InteractBlock]: ", pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }
            if (!InteractBlockMap.contains(block)) {
                return true;
            }

            auto const& tab = land->getLandPermTableConst();
            if (block == "minecraft:cartography_table" && tab.useCartographyTable) return true; // 制图台
            if (block == "minecraft:smithing_table" && tab.useSmithingTable) return true;       // 锻造台
            if (block == "minecraft:brewing_stand" && tab.useBrewingStand) return true;         // 酿造台
            if (block == "minecraft:anvil" && tab.useAnvil) return true;                        // 铁砧
            if (block == "minecraft:grindstone" && tab.useGrindstone) return true;              // 磨石
            if (block == "minecraft:enchanting_table" && tab.useEnchantingTable) return true;   // 附魔台
            if (block == "minecraft:barrel" && tab.useBarrel) return true;                      // 桶
            if (block == "minecraft:beacon" && tab.useBeacon) return true;                      // 信标
            if (block == "minecraft:hopper" && tab.useHopper) return true;                      // 漏斗
            if (block == "minecraft:dropper" && tab.useDropper) return true;                    // 投掷器
            if (block == "minecraft:dispenser" && tab.useDispenser) return true;                // 发射器
            if (block == "minecraft:loom" && tab.useLoom) return true;                          // 织布机
            if (block == "minecraft:stonecutter_block" && tab.useStonecutter) return true;      // 切石机
            if (block.ends_with("blast_furnace") && tab.useBlastFurnace) return true;           // 高炉
            if (block.ends_with("furnace") && tab.useFurnace) return true;                      // 熔炉
            if (block.ends_with("smoker") && tab.useSmoker) return true;                        // 烟熏炉

            ev.cancel();
            return true;
        });

    mPlayerUseItemEvent =
        bus->emplaceListener<ll::event::PlayerUseItemEvent>([db, logger](ll::event::PlayerUseItemEvent& ev) {
            if (!ev.item().getTypeName().ends_with("bucket")) {
                return true;
            }

            auto& player = ev.self();
            auto  val    = player.traceRay(5.5f, false, true, [&](BlockSource const&, Block const& bl, bool) {
                if (bl.getMaterial().isLiquid()) return false; // 液体方块
                return true;
            });

            BlockPos const&  pos   = val.mBlock;
            ItemStack const& item  = ev.item();
            Block const&     block = player.getDimensionBlockSource().getBlock(pos);

            logger->debug(
                "[UseItem] Item: {}, Pos: {}, Block: {}",
                item.getTypeName(),
                pos.toString(),
                block.getTypeName()
            );

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            // 防止玩家在可含水方块里放水
            if (BlockLiquidDetectionComponent::canContainLiquid(block)) {
                ev.cancel();
                static uchar flags = (1 << 0) | (1 << 1); // 0b11 BlockUpdateFlag::All v0.13.5
                UpdateBlockPacket(pos, (uint)SubChunk::BlockLayer::Extra, block.getBlockItemId(), flags).sendTo(player);
                return true;
            };
            return true;
        });

    // Minecraft events (MoreEvents)
    mPlayerAttackBlockEvent = bus->emplaceListener<ila::mc::PlayerAttackBlockBeforeEvent>(
        [db, logger](ila::mc::PlayerAttackBlockBeforeEvent& ev) {
            optional_ref<Player> pl = ev.self();
            if (!pl.has_value()) return true;

            Player& player = pl.value();

            logger->debug("[AttackBlock] {}", ev.getPos().toString());

            auto land = db->getLandAt(ev.getPos(), player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            auto const& bl = player.getDimensionBlockSourceConst().getBlock(ev.getPos()).getTypeName();
            if (land->getLandPermTableConst().allowAttackDragonEgg && bl == "minecraft:dragon_egg") return true;

            ev.cancel();
            return true;
        }
    );

    mArmorStandSwapItemEvent = bus->emplaceListener<ila::mc::ArmorStandSwapItemBeforeEvent>(
        [db, logger](ila::mc::ArmorStandSwapItemBeforeEvent& ev) {
            Player& player = ev.getPlayer();

            logger->debug("[ArmorStandSwapItem]: executed");

            auto land = db->getLandAt(ev.self().getPosition(), player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            if (land->getLandPermTableConst().useArmorStand) return true;

            ev.cancel();
            return true;
        }
    );

    mPlayerDropItemEvent =
        bus->emplaceListener<ila::mc::PlayerDropItemBeforeEvent>([db, logger](ila::mc::PlayerDropItemBeforeEvent& ev) {
            Player& player = ev.self();

            logger->debug("[PlayerDropItem]: executed");

            auto land = db->getLandAt(player.getPosition(), player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return true;
            }

            if (land->getLandPermTableConst().allowDropItem) return true;

            ev.cancel();
            return true;
        });

    mActorRideEvent =
        bus->emplaceListener<ila::mc::ActorRideBeforeEvent>([db, logger](ila::mc::ActorRideBeforeEvent& ev) {
            logger->debug("[ActorRide]: executed");
            Actor& passenger = ev.self();

            if (!passenger.isPlayer()) {
                return true; // 忽略非玩家骑乘事件
            }

            auto const& typeName = ev.getTarget().getTypeName();
            auto        land     = db->getLandAt(passenger.getPosition(), passenger.getDimensionId());
            if (PreCheck(land)) return true; // land not found
            // 特殊处理：
            if (land) {
                auto& tab = land->getLandPermTableConst();
                if (typeName == "minecraft:minecart" || typeName == "minecraft:boat") {
                    if (tab.allowRideTrans) return true;
                } else {
                    if (tab.allowRideEntity) return true;
                }
            }

            if (passenger.isPlayer()) {
                auto player = passenger.getWeakEntity().tryUnwrap<Player>();
                if (player.has_value() && PreCheck(land, player->getUuid().asString())) {
                    return true;
                }
            }

            ev.cancel();
            return true;
        });

    mExplodeEvent =
        bus->emplaceListener<ila::mc::ExplosionBeforeEvent>([db, logger](ila::mc::ExplosionBeforeEvent& ev) {
            logger->debug("[Explode] Pos: {}", ev.getExplosion().mPos->toString());

            auto lands = db->getLandAt(
                BlockPos{ev.getExplosion().mPos},
                (int)(ev.getExplosion().mRadius + 1.0),
                ev.blockSource().getDimensionId()
            );
            for (auto& p : lands) {
                if (!p->getLandPermTableConst().allowExplode) {
                    ev.cancel();
                    break;
                }
            }

            return true;
        });

    mFarmDecayEvent =
        bus->emplaceListener<ila::mc::FarmDecayBeforeEvent>([db, logger](ila::mc::FarmDecayBeforeEvent& ev) {
            logger->debug("[FarmDecay] Pos: {}", ev.getPos().toString());

            auto land = db->getLandAt(ev.getPos(), ev.blockSource().getDimensionId());
            if (PreCheck(land)) return true; // land not found
            if (land) {
                if (land->getLandPermTableConst().allowFarmDecay) return true;
            }

            ev.cancel();
            return true;
        });

    mMobHurtEffectEvent =
        bus->emplaceListener<ila::mc::MobHurtEffectBeforeEvent>([db, logger](ila::mc::MobHurtEffectBeforeEvent& ev) {
            logger->debug("[MobHurtEffect] mob: {}", ev.self().getTypeName());
            auto& self = ev.self();

            auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
            if (PreCheck(land)) return true; // land not found
            if (land) {
                auto const& et  = self.getTypeName();
                auto const& tab = land->getLandPermTableConst();
                if (tab.allowAttackPlayer && self.isPlayer()) return true;
                if (tab.allowAttackAnimal && AnimalEntityMap.contains(et)) return true;
                if (tab.allowAttackMob && !AnimalEntityMap.contains(et)) return true;
            }

            if (self.isPlayer()) {
                auto const pl = self.getWeakEntity().tryUnwrap<Player>();
                if (pl.has_value()) {
                    if (PreCheck(land, pl->getUuid().asString())) return true;
                }
            }

            ev.cancel();
            return true;
        });

    mPistonTryPushEvent =
        bus->emplaceListener<ila::mc::PistonPushBeforeEvent>([db, logger](ila::mc::PistonPushBeforeEvent& ev) {
            auto const& piston = ev.getPistonPos();
            auto const& push   = ev.getPushPos();
            auto&       region = ev.blockSource();

            logger->debug("[PistonTryPush] piston: {}, push: {}", piston.toString(), push.toString());

            auto land  = db->getLandAt(push, region.getDimensionId());
            auto land2 = db->getLandAt(piston, region.getDimensionId());
            if (land && !land->getLandPermTableConst().allowPistonPush && land != land2) {
                ev.cancel();
            }
        });

    mPlayerUseItemFrameEvent = bus->emplaceListener<ila::mc::PlayerOperatedItemFrameBeforeEvent>(
        [db, logger](ila::mc::PlayerOperatedItemFrameBeforeEvent& ev) {
            logger->debug("[PlayerUseItemFrame] pos: {}", ev.getBlockPos().toString());

            auto land = db->getLandAt(ev.getBlockPos(), ev.self().getDimensionId());
            if (PreCheck(land, ev.self().getUuid().asString())) return;

            if (land->getLandPermTableConst().useItemFrame) return;

            ev.cancel();
        }
    );

    mPressurePlateTriggerEvent = bus->emplaceListener<ila::mc::ActorTriggerPressurePlateBeforeEvent>(
        [db, logger](ila::mc::ActorTriggerPressurePlateBeforeEvent& ev) {
            logger->debug("[PressurePlateTrigger] pos: {}", ev.getPos().toString());

            auto land = db->getLandAt(ev.getPos(), ev.self().getDimensionId());
            if (land && land->getLandPermTableConst().usePressurePlate) return;
            if (PreCheck(land)) return; // land not found

            auto& entity = ev.self();
            if (entity.isPlayer()) {
                auto pl = entity.getWeakEntity().tryUnwrap<Player>();
                if (pl.has_value()) {
                    if (PreCheck(land, pl->getUuid().asString())) return;
                }
            }

            ev.cancel();
            return;
        }
    );

    mProjectileSpawnEvent =
        bus->emplaceListener<ila::mc::ProjectileCreateBeforeEvent>([db,
                                                                    logger](ila::mc::ProjectileCreateBeforeEvent& ev) {
            logger->debug("[ProjectileSpawn] type: {}", ev.self().getTypeName());
            Actor* actor = &ev.self();
            if (!actor) {
                return;
            }

            auto& type = ev.self().getTypeName();
            auto  land = db->getLandAt(actor->getPosition(), actor->getDimensionId());
            if (PreCheck(land)) return; // land not found
            if (land) {
                auto& tab = land->getLandPermTableConst();
                if (type == "minecraft:fishing_hook" && tab.useFishingHook) return;       // 钓鱼竿
                if (type == "minecraft:splash_potion" && tab.allowThrowPotion) return;    // 喷溅药水
                if (type == "minecraft:lingering_potion" && tab.allowThrowPotion) return; // 滞留药水
                if (type == "minecraft:thrown_trident" && tab.allowThrowTrident) return;  // 三叉戟
                if (type == "minecraft:arrow" && tab.allowShoot) return;                  // 箭
                if (type == "minecraft:crossbow" && tab.allowShoot) return;               // 弩射烟花
                if (type == "minecraft:snowball" && tab.allowThrowSnowball) return;       // 雪球
                if (type == "minecraft:ender_pearl" && tab.allowThrowEnderPearl) return;  // 末影珍珠
                if (type == "minecraft:egg" && tab.allowThrowEgg) return;                 // 鸡蛋
            }

            if (actor->isPlayer()) {
                auto pl = actor->getWeakEntity().tryUnwrap<Player>();
                if (pl.has_value()) {
                    if (PreCheck(land, pl->getUuid().asString())) return;
                }
            }

            ev.cancel();
            return;
        });

    mRedstoneUpdateEvent =
        bus->emplaceListener<ila::mc::RedstoneUpdateBeforeEvent>([db, logger](ila::mc::RedstoneUpdateBeforeEvent& ev) {
            logger->debug("[RedstoneUpdate] Pos: {}", ev.getPos().toString());

            auto land = db->getLandAt(ev.getPos(), ev.blockSource().getDimensionId());
            if (PreCheck(land)) return true; // land not found
            if (land) {
                if (land->getLandPermTableConst().allowRedstoneUpdate) return true;
            }

            ev.cancel();
            return true;
        });

    mWitherDestroyBlockEvent =
        bus->emplaceListener<ila::mc::WitherDestroyBeforeEvent>([db, logger](ila::mc::WitherDestroyBeforeEvent& ev) {
            logger->debug("[WitherDestroyBlock] executed");
            auto& aabb = ev.getBox();

            auto lands = db->getLandAt(aabb.min, aabb.max, ev.blockSource().getDimensionId());
            for (auto const& p : lands) {
                if (!p->getLandPermTableConst().allowWitherDestroy) {
                    ev.cancel();
                    break;
                }
            }

            return true;
        });

    mMossFertilizerEvent =
        bus->emplaceListener<ila::mc::MossGrowthBeforeEvent>([db, logger](ila::mc::MossGrowthBeforeEvent& ev) {
            logger->debug("[MossSpread] {}", ev.getPos().toString());

            auto const& pos = ev.getPos();

            auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (!land || land->getLandPermTableConst().useBoneMeal) {
                return;
            }

            auto lds = db->getLandAt(pos - 2, pos + 2, ev.blockSource().getDimensionId());
            for (auto const& p : lds) {
                if (p->getLandPermTableConst().useBoneMeal) {
                    return;
                }
            }

            ev.cancel();
        });

    mLiquidFlowEvent =
        bus->emplaceListener<ila::mc::LiquidTryFlowBeforeEvent>([db, logger](ila::mc::LiquidTryFlowBeforeEvent& ev) {
            auto& sou = ev.getPos();
            // auto& from = ev.getFlowFromPos();
            // logger->debug("[LiquidFlow] {} -> {}", sou.toString(), from.toString());

            auto land = db->getLandAt(sou, ev.blockSource().getDimensionId());
            if (land) {
                if (!land->getLandPermTableConst().allowLiquidFlow) {
                    ev.cancel();
                    return;
                }
            }
        });

    // mSculkCatalystAbsorbExperienceEvent = bus->emplaceListener<more_events::SculkCatalystAbsorbExperienceEvent>(
    //     [db, logger](more_events::SculkCatalystAbsorbExperienceEvent& ev) {
    //         auto& actor  = ev.getDiedActor();
    //         auto& region = actor.getDimensionBlockSource();
    //         auto  pos    = actor.getBlockPosCurrentlyStandingOn(&actor);
    //         logger->debug("[SculkCatalystAbsorbExperience] Pos: {}", pos.toString());

    //         // 领地内蔓延 && 半径内没有别的领地 => 放行
    //         // 领地外蔓延 && 半径内有别的领地   => 放行

    //         auto cur = db->getLandAt(pos, region.getDimensionId());
    //         auto lds = db->getLandAt(pos - 9, pos + 9, region.getDimensionId());

    //         if (cur && lds.size() == 1) return;
    //         if (!cur && lds.empty()) return;

    //         ev.cancel();
    //     }
    // );

    mSculkBlockGrowthEvent =
        bus->emplaceListener<ila::mc::SculkBlockGrowthBeforeEvent>([db,
                                                                    logger](ila::mc::SculkBlockGrowthBeforeEvent& ev) {
            auto& pos = ev.getPos();
            logger->debug("[SculkBlockGrowth] {}", pos.toString());

            auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (land) {
                if (!land->getLandPermTableConst().allowSculkBlockGrowth) {
                    ev.cancel();
                }
            }
        });

    mSculkSpreadEvent =
        bus->emplaceListener<ila::mc::SculkSpreadBeforeEvent>([db, logger](ila::mc::SculkSpreadBeforeEvent& ev) {
            logger->debug("[SculkSpread] {} -> {}", ev.getSelfPos().toString(), ev.getTargetPos().toString());

            auto sou = db->getLandAt(ev.getSelfPos(), ev.blockSource().getDimensionId());
            auto tar = db->getLandAt(ev.getTargetPos(), ev.blockSource().getDimensionId());

            if (!sou && !tar) return; // 领地外蔓延
            if (sou && tar) return;   // 领地内蔓延
            if (sou && !tar) return;  // 领地内蔓延到外
            if (!sou && tar) {
                ev.cancel(); // 外蔓延到领地内
            }
        });

    return true;
}


bool EventListener::release() {
    auto& bus = ll::event::EventBus::getInstance();

    bus.removeListener(mPlayerJoinEvent);
    bus.removeListener(mActorHurtEvent);
    bus.removeListener(mPlayerDestroyBlockEvent);
    bus.removeListener(mPlayerPlaceingBlockEvent);
    bus.removeListener(mPlayerUseItemOnEvent);
    bus.removeListener(mFireSpreadEvent);
    bus.removeListener(mPlayerAttackEntityEvent);
    bus.removeListener(mPlayerPickUpItemEvent);
    bus.removeListener(mPlayerInteractBlockEvent);
    bus.removeListener(mPlayerUseItemEvent);

    bus.removeListener(mArmorStandSwapItemEvent);
    bus.removeListener(mPlayerAttackBlockEvent);
    bus.removeListener(mPlayerDropItemEvent);
    bus.removeListener(mActorRideEvent);
    bus.removeListener(mExplodeEvent);
    bus.removeListener(mFarmDecayEvent);
    bus.removeListener(mMobHurtEffectEvent);
    bus.removeListener(mPistonTryPushEvent);
    bus.removeListener(mPlayerUseItemFrameEvent);
    bus.removeListener(mPressurePlateTriggerEvent);
    bus.removeListener(mProjectileSpawnEvent);
    bus.removeListener(mRedstoneUpdateEvent);
    bus.removeListener(mWitherDestroyBlockEvent);
    bus.removeListener(mMossFertilizerEvent);
    bus.removeListener(mLiquidFlowEvent);
    // bus.removeListener(mSculkCatalystAbsorbExperienceEvent);
    bus.removeListener(mSculkBlockGrowthEvent);
    bus.removeListener(mSculkSpreadEvent);

    return true;
}


// static
std::unordered_set<string> EventListener::UseItemOnMap = {
    "minecraft:bed",                        // 床
    "minecraft:chest",                      // 箱子
    "minecraft:trapped_chest",              // 陷阱箱
    "minecraft:crafting_table",             // 制作台
    "minecraft:campfire",                   // 营火
    "minecraft:soul_campfire",              // 灵魂营火
    "minecraft:composter",                  // 垃圾箱
    "minecraft:noteblock",                  // 音符盒
    "minecraft:jukebox",                    // 唱片机
    "minecraft:bell",                       // 钟
    "minecraft:daylight_detector",          // 阳光探测器
    "minecraft:daylight_detector_inverted", // 阳光探测器(夜晚)
    "minecraft:lectern",                    // 讲台
    "minecraft:cauldron",                   // 炼药锅
    "minecraft:lever",                      // 拉杆
    "minecraft:dragon_egg",                 // 龙蛋
    "minecraft:flower_pot",                 // 花盆
    "minecraft:respawn_anchor",             // 重生锚
    "minecraft:glow_ink_sac",               // 荧光墨囊
    "minecraft:end_crystal",                // 末地水晶
    "minecraft:ender_eye",                  // 末影之眼
    "minecraft:flint_and_steel",            // 打火石
    "minecraft:skull",                      // 头颅
    "minecraft:banner",                     // 旗帜
    "minecraft:bone_meal",                  // 骨粉
    "minecraft:minecart",                   // 矿车
    "minecraft:armor_stand",                // 盔甲架

    "minecraft:axolotl_bucket",       // 美西螈桶
    "minecraft:powder_snow_bucket",   // 细雪桶
    "minecraft:pufferfish_bucket",    // 河豚桶
    "minecraft:tropical_fish_bucket", // 热带鱼桶
    "minecraft:salmon_bucket",        // 桶装鲑鱼
    "minecraft:cod_bucket",           // 鳕鱼桶
    "minecraft:water_bucket",         // 水桶
    "minecraft:cod_bucket",           // 鳕鱼桶
    "minecraft:lava_bucket",          // 熔岩桶
    "minecraft:bucket",               // 桶

    "minecraft:shulker_box",            // 潜影盒
    "minecraft:undyed_shulker_box",     // 未染色的潜影盒
    "minecraft:white_shulker_box",      // 白色潜影盒
    "minecraft:orange_shulker_box",     // 橙色潜影盒
    "minecraft:magenta_shulker_box",    // 品红色潜影盒
    "minecraft:light_blue_shulker_box", // 浅蓝色潜影盒
    "minecraft:yellow_shulker_box",     // 黄色潜影盒
    "minecraft:lime_shulker_box",       // 黄绿色潜影盒
    "minecraft:pink_shulker_box",       // 粉红色潜影盒
    "minecraft:gray_shulker_box",       // 灰色潜影盒
    "minecraft:light_gray_shulker_box", // 浅灰色潜影盒
    "minecraft:cyan_shulker_box",       // 青色潜影盒
    "minecraft:purple_shulker_box",     // 紫色潜影盒
    "minecraft:blue_shulker_box",       // 蓝色潜影盒
    "minecraft:brown_shulker_box",      // 棕色潜影盒
    "minecraft:green_shulker_box",      // 绿色潜影盒
    "minecraft:red_shulker_box",        // 红色潜影盒
    "minecraft:black_shulker_box",      // 黑色潜影盒

    "minecraft:stone_button",               // 石头按钮
    "minecraft:wooden_button",              // 木头按钮
    "minecraft:spruce_button",              // 云杉木按钮
    "minecraft:birch_button",               // 白桦木按钮
    "minecraft:jungle_button",              // 丛林木按钮
    "minecraft:acacia_button",              // 金合欢木按钮
    "minecraft:dark_oak_button",            // 深色橡木按钮
    "minecraft:crimson_button",             // 绯红木按钮
    "minecraft:warped_button",              // 诡异木按钮
    "minecraft:polished_blackstone_button", // 磨制黑石按钮
    "minecraft:mangrove_button",            // 红树木按钮
    "minecraft:cherry_button",              // 樱花木按钮
    "minecraft:bamboo_button",              // 竹按钮

    "minecraft:trapdoor",                        // 活板门
    "minecraft:spruce_trapdoor",                 // 云杉木活板门
    "minecraft:birch_trapdoor",                  // 白桦木活板门
    "minecraft:jungle_trapdoor",                 // 丛林木活板门
    "minecraft:acacia_trapdoor",                 // 金合欢木活板门
    "minecraft:dark_oak_trapdoor",               // 深色橡木活板门
    "minecraft:crimson_trapdoor",                // 绯红木活板门
    "minecraft:warped_trapdoor",                 // 诡异木活板门
    "minecraft:copper_trapdoor",                 // 铜活板门
    "minecraft:exposed_copper_trapdoor",         // 斑驳的铜活板门
    "minecraft:weathered_copper_trapdoor",       // 锈蚀的铜活板门
    "minecraft:oxidized_copper_trapdoor",        // 氧化的铜活板门
    "minecraft:waxed_copper_trapdoor",           // 涂蜡的铜活板门
    "minecraft:waxed_exposed_copper_trapdoor",   // 涂蜡的斑驳的铜活板门
    "minecraft:waxed_weathered_copper_trapdoor", // 涂蜡的锈蚀的铜活板门
    "minecraft:waxed_oxidized_copper_trapdoor",  // 涂蜡的氧化的铜活板门
    "minecraft:mangrove_trapdoor",               // 红树木活板门
    "minecraft:cherry_trapdoor",                 // 樱树木活板门
    "minecraft:bamboo_trapdoor",                 // 竹活板门

    "minecraft:fence_gate",          // 栅栏门
    "minecraft:spruce_fence_gate",   // 云杉木栅栏门
    "minecraft:birch_fence_gate",    // 白桦木栅栏门
    "minecraft:jungle_fence_gate",   // 丛林木栅栏门
    "minecraft:acacia_fence_gate",   // 金合欢木栅栏门
    "minecraft:dark_oak_fence_gate", // 深色橡木栅栏门
    "minecraft:crimson_fence_gate",  // 绯红木栅栏门
    "minecraft:warped_fence_gate",   // 诡异木栅栏门
    "minecraft:mangrove_fence_gate", // 红树木栅栏门
    "minecraft:cherry_fence_gate",   // 樱树木栅栏门
    "minecraft:bamboo_fence_gate",   // 竹栅栏门

    "minecraft:wooden_door",   // 橡木门
    "minecraft:spruce_door",   // 云杉木门
    "minecraft:birch_door",    // 白桦木门
    "minecraft:jungle_door",   // 丛林木门
    "minecraft:acacia_door",   // 金合欢木门
    "minecraft:dark_oak_door", // 深色橡木门
    "minecraft:crimson_door",  // 绯红木门
    "minecraft:warped_door",   // 诡异木门
    "minecraft:mangrove_door", // 红树木门
    "minecraft:cherry_door",   // 樱树木门
    "minecraft:bamboo_door",   // 竹门

    "minecraft:wooden_axe",       // 木斧
    "minecraft:stone_axe",        // 石斧
    "minecraft:iron_axe",         // 铁斧
    "minecraft:golden_axe",       // 金斧
    "minecraft:diamond_axe",      // 钻石斧
    "minecraft:netherite_axe",    // 下界合金斧
    "minecraft:wooden_hoe",       // 木锄
    "minecraft:stone_hoe",        // 石锄
    "minecraft:iron_hoe",         // 铁锄
    "minecraft:diamond_hoe",      // 钻石锄
    "minecraft:golden_hoe",       // 金锄
    "minecraft:netherite_hoe",    // 下界合金锄
    "minecraft:wooden_shovel",    // 木铲
    "minecraft:stone_shovel",     // 石铲
    "minecraft:iron_shovel",      // 铁铲
    "minecraft:diamond_shovel",   // 钻石铲
    "minecraft:golden_shovel",    // 金铲
    "minecraft:netherite_shovel", // 下界合金铲

    "minecraft:standing_sign",          // 站立的告示牌
    "minecraft:spruce_standing_sign",   // 站立的云杉木告示牌
    "minecraft:birch_standing_sign",    // 站立的白桦木告示牌
    "minecraft:jungle_standing_sign",   // 站立的丛林木告示牌
    "minecraft:acacia_standing_sign",   // 站立的金合欢木告示牌
    "minecraft:darkoak_standing_sign",  // 站立的深色橡木告示牌
    "minecraft:mangrove_standing_sign", // 站立的红树木告示牌
    "minecraft:cherry_standing_sign",   // 站立的樱树木告示牌
    "minecraft:bamboo_standing_sign",   // 站立的竹子告示牌
    "minecraft:crimson_standing_sign",  // 站立的绯红木告示牌
    "minecraft:warped_standing_sign",   // 站立的诡异木告示牌
    "minecraft:wall_sign",              // 墙上的告示牌
    "minecraft:spruce_wall_sign",       // 墙上的云杉木告示牌
    "minecraft:birch_wall_sign",        // 墙上的白桦木告示牌
    "minecraft:jungle_wall_sign",       // 墙上的丛林木告示牌
    "minecraft:acacia_wall_sign",       // 墙上的金合欢木告示牌
    "minecraft:darkoak_wall_sign",      // 墙上的深色橡木告示牌
    "minecraft:mangrove_wall_sign",     // 墙上的红树木告示牌
    "minecraft:cherry_wall_sign",       // 墙上的樱树木告示牌
    "minecraft:bamboo_wall_sign",       // 墙上的竹子告示牌
    "minecraft:crimson_wall_sign",      // 墙上的绯红木告示牌
    "minecraft:warped_wall_sign"        // 墙上的诡异木告示牌
};
std::unordered_set<string> EventListener::InteractBlockMap = {
    "minecraft:cartography_table", // 制图台
    "minecraft:smithing_table",    // 锻造台
    "minecraft:furnace",           // 熔炉
    "minecraft:blast_furnace",     // 高炉
    "minecraft:smoker",            // 烟熏炉
    "minecraft:brewing_stand",     // 酿造台
    "minecraft:anvil",             // 铁砧
    "minecraft:grindstone",        // 砂轮
    "minecraft:enchanting_table",  // 附魔台
    "minecraft:barrel",            // 木桶
    "minecraft:beacon",            // 信标
    "minecraft:hopper",            // 漏斗
    "minecraft:dropper",           // 投掷器
    "minecraft:dispenser",         // 发射器
    "minecraft:loom",              // 织布机
    "minecraft:stonecutter_block", // 切石机
    "minecraft:lit_furnace",       // 燃烧中的熔炉
    "minecraft:lit_blast_furnace", // 燃烧中的高炉
    "minecraft:lit_smoker"         // 燃烧中的烟熏炉
};
std::unordered_set<string> EventListener::AnimalEntityMap = {
    "minecraft:axolotl",          // 美西螈
    "minecraft:bat",              // 蝙蝠
    "minecraft:cat",              // 猫
    "minecraft:chicken",          // 鸡
    "minecraft:cod",              // 鳕鱼
    "minecraft:cow",              // 牛
    "minecraft:donkey",           // 驴
    "minecraft:fox",              // 狐狸
    "minecraft:glow_squid",       // 发光鱿鱼
    "minecraft:horse",            // 马
    "minecraft:mooshroom",        // 蘑菇牛
    "minecraft:mule",             // 驴
    "minecraft:ocelot",           // 豹猫
    "minecraft:parrot",           // 鹦鹉
    "minecraft:pig",              // 猪
    "minecraft:rabbit",           // 兔子
    "minecraft:salmon",           // 鲑鱼
    "minecraft:snow_golem",       // 雪傀儡
    "minecraft:sheep",            // 羊
    "minecraft:skeleton_horse",   // 骷髅马
    "minecraft:squid",            // 鱿鱼
    "minecraft:strider",          // 炽足兽
    "minecraft:tropical_fish",    // 热带鱼
    "minecraft:turtle",           // 海龟
    "minecraft:villager_v2",      // 村民
    "minecraft:wandering_trader", // 流浪商人
    "minecraft:npc"               // NPC
};


} // namespace land
