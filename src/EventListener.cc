#include "pland/EventListener.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/event/world/SpawnMobEvent.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/shared_types/legacy/actor/ActorDamageCause.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/BlockLegacy.h" 
#include "mc/world/level/block/BlockProperty.h"
#include "mc/world/item/Item.h" 
#include "mc/world/item/BucketItem.h"  
#include "mc/world/item/HatchetItem.h" 
#include "mc/world/item/HoeItem.h"     
#include "mc/world/item/ShovelItem.h" 
#include "mc/world/level/block/SignBlock.h"
#include "mc/world/level/block/HangingSignBlock.h"
#include "mc/world/level/block/ShulkerBoxBlock.h"
#include "mc/world/level/block/FurnaceBlock.h"
#include "mc/world/level/block/BlastFurnaceBlock.h"
#include "mc/world/level/block/SmokerBlock.h"
#include "mc/world/level/block/components/BlockComponentDirectData.h"
#include "mc/world/level/chunk/SubChunk.h"
#include "mc/world/phys/AABB.h"
#include "mc/world/phys/HitResult.h"
#include "mod/MyMod.h"
#include "pland/Config.h"
#include "pland/DrawHandleManager.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandScheduler.h"
#include "pland/LandSelector.h"
#include "pland/PLand.h"
#include "pland/utils/McUtils.h"
#include <cstdint>
#include <unordered_set>
#include <vector>
#include <functional> 
#include <string_view> 


#include "ll/api/event/entity/ActorHurtEvent.h"
#include "ll/api/event/player/PlayerAttackEvent.h"
#include "ll/api/event/player/PlayerDestroyBlockEvent.h"
#include "ll/api/event/player/PlayerDisconnectEvent.h"
#include "ll/api/event/player/PlayerInteractBlockEvent.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/event/player/PlayerPickUpItemEvent.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "ll/api/event/world/FireSpreadEvent.h"


#include "ila/event/minecraft/world/ExplosionEvent.h"
#include "ila/event/minecraft/world/PistonPushEvent.h"
#include "ila/event/minecraft/world/RedstoneUpdateEvent.h"
#include "ila/event/minecraft/world/SculkBlockGrowthEvent.h"
#include "ila/event/minecraft/world/WitherDestroyEvent.h"
#include "ila/event/minecraft/world/actor/ActorRideEvent.h"
#include "ila/event/minecraft/world/actor/ActorTriggerPressurePlateEvent.h"
#include "ila/event/minecraft/world/actor/ArmorStandSwapItemEvent.h"
#include "ila/event/minecraft/world/actor/MobHurtEffectEvent.h"
#include "ila/event/minecraft/world/actor/ProjectileCreateEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerAttackBlockEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerDropItemEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerEditSignEvent.h"
#include "ila/event/minecraft/world/actor/player/PlayerOperatedItemFrameEvent.h"
#include "ila/event/minecraft/world/level/block/FarmDecayEvent.h"
#include "ila/event/minecraft/world/level/block/LiquidTryFlowEvent.h"
#include "ila/event/minecraft/world/level/block/MossGrowthEvent.h"
#include "ila/event/minecraft/world/level/block/SculkCatalystAbsorbExperienceEvent.h"
#include "ila/event/minecraft/world/level/block/SculkSpreadEvent.h"

// Fix BlockProperty operator&
inline BlockProperty operator&(BlockProperty a, BlockProperty b) {
    return static_cast<BlockProperty>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

#define CANCEL_AND_RETURN_IF(COND)                                                                                     \
    if (COND) {                                                                                                        \
        ev.cancel();                                                                                                   \
        return;                                                                                                        \
    }

#define CHECK_EVENT_AND_REGISTER_LISTENER(EXPRESSION, LISTENER)                                                        \
    if (EXPRESSION) {                                                                                                  \
        mListeners.push_back(LISTENER);                                                                                \
    }


static std::vector<ll::event::ListenerPtr> mListeners;

namespace land {
inline bool PreCheck(LandData_sptr const& ptr, UUIDs const& uuid = "", bool ignoreOperator = false) {
    if (!ptr ||                                                       // 无领地
        (!ignoreOperator && PLand::getInstance().isOperator(uuid)) || // 管理员
        (ptr->getPermType(uuid) != LandPermType::Guest)               // 主人/成员
    ) {
        return true;
    }
    return false;
}

// --- 权限检查辅助查找表 ---

// PlayerInteractBlockEvent 中特定物品交互的权限映射表
static const std::unordered_map<std::string_view, bool LandPermTable::*> itemSpecificPermissionMap = {
    {"minecraft:skull",           &LandPermTable::allowPlace},           // 放置头颅
    {"minecraft:banner",          &LandPermTable::allowPlace},           // 放置旗帜
    {"minecraft:glow_ink_sac",    &LandPermTable::allowPlace},           // 荧光墨囊给告示牌上色
    {"minecraft:end_crystal",     &LandPermTable::allowPlace},           // 放置末地水晶
    {"minecraft:ender_eye",       &LandPermTable::allowPlace},           // 放置末影之眼
    {"minecraft:flint_and_steel", &LandPermTable::useFiregen},           // 使用打火石
    {"minecraft:bone_meal",       &LandPermTable::useBoneMeal},          // 使用骨粉
    {"minecraft:minecart",        &LandPermTable::allowPlace},           // 放置矿车
    {"minecraft:armor_stand",     &LandPermTable::allowPlace}            // 放置盔甲架
};

// PlayerInteractBlockEvent 中特定方块交互的权限映射表 (当物品名称也可能是方块名称时，且检查逻辑特殊)
static const std::unordered_map<std::string_view, bool LandPermTable::*> blockSpecificPermissionMap = {
    {"minecraft:dragon_egg",    &LandPermTable::allowAttackDragonEgg}, // 攻击龙蛋
    {"minecraft:bed",           &LandPermTable::useBed},               // 使用床
    {"minecraft:chest",         &LandPermTable::allowOpenChest},       // 打开箱子
    {"minecraft:trapped_chest", &LandPermTable::allowOpenChest},       // 打开陷阱箱
    {"minecraft:campfire",      &LandPermTable::useCampfire},          // 使用营火 
    {"minecraft:soul_campfire", &LandPermTable::useCampfire},          // 使用灵魂营火
    {"minecraft:composter",     &LandPermTable::useComposter},         // 使用堆肥桶
    {"minecraft:noteblock",     &LandPermTable::useNoteBlock},         // 使用音符盒
    {"minecraft:jukebox",       &LandPermTable::useJukebox},           // 使用唱片机
    {"minecraft:bell",          &LandPermTable::useBell},              // 使用钟
    {"minecraft:daylight_detector_inverted", &LandPermTable::useDaylightDetector}, // 使用阳光探测器 (反向)
    {"minecraft:daylight_detector", &LandPermTable::useDaylightDetector},          // 使用阳光探测器 
    {"minecraft:lectern",       &LandPermTable::useLectern},           // 使用讲台
    {"minecraft:cauldron",      &LandPermTable::useCauldron},          // 使用炼药锅
    {"minecraft:respawn_anchor",&LandPermTable::useRespawnAnchor},    // 使用重生锚
    {"minecraft:flower_pot",    &LandPermTable::editFlowerPot}         // 编辑花盆
};

// PlayerInteractBlockEvent 中功能性方块交互的权限映射表 (对应 InteractBlockHashMap)
static const std::unordered_map<std::string_view, bool LandPermTable::*> blockFunctionalPermissionMap = {
    {"minecraft:cartography_table", &LandPermTable::useCartographyTable}, // 制图台
    {"minecraft:smithing_table",    &LandPermTable::useSmithingTable},    // 锻造台
    {"minecraft:brewing_stand",     &LandPermTable::useBrewingStand},     // 酿造台
    {"minecraft:anvil",             &LandPermTable::useAnvil},            // 铁砧
    {"minecraft:grindstone",        &LandPermTable::useGrindstone},       // 砂轮
    {"minecraft:enchanting_table",  &LandPermTable::useEnchantingTable},  // 附魔台
    {"minecraft:barrel",            &LandPermTable::useBarrel},           // 木桶
    {"minecraft:beacon",            &LandPermTable::useBeacon},           // 信标
    {"minecraft:hopper",            &LandPermTable::useHopper},           // 漏斗
    {"minecraft:dropper",           &LandPermTable::useDropper},          // 投掷器
    {"minecraft:dispenser",         &LandPermTable::useDispenser},        // 发射器
    {"minecraft:loom",              &LandPermTable::useLoom},             // 织布机
    {"minecraft:stonecutter_block", &LandPermTable::useStonecutter}       // 切石机
    // 熔炉相关的方块由下面的 ends_with 处理
};

// ProjectileCreateBeforeEvent 中抛射物创建的权限映射表
static const std::unordered_map<std::string_view, bool LandPermTable::*> projectilePermissionMap = {
    {"minecraft:fishing_hook",     &LandPermTable::useFishingHook},       // 鱼钩
    {"minecraft:splash_potion",    &LandPermTable::allowThrowPotion},    // 喷溅药水
    {"minecraft:lingering_potion", &LandPermTable::allowThrowPotion},    // 滞留药水
    {"minecraft:thrown_trident",   &LandPermTable::allowThrowTrident},  // 三叉戟
    {"minecraft:arrow",            &LandPermTable::allowShoot},          // 箭
    {"minecraft:crossbow",         &LandPermTable::allowShoot},          // 弩 (假设发射箭或烟花需要 allowShoot 权限)
    {"minecraft:snowball",         &LandPermTable::allowThrowSnowball},  // 雪球
    {"minecraft:ender_pearl",      &LandPermTable::allowThrowEnderPearl}, // 末影珍珠
    {"minecraft:egg",              &LandPermTable::allowThrowEgg}         // 鸡蛋
};


bool EventListener::setup() {
    auto* db     = &PLand::getInstance();
    auto* bus    = &ll::event::EventBus::getInstance();
    auto* logger = &my_mod::MyMod::getInstance().getSelf().getLogger();

    mListeners = {
        bus->emplaceListener<ll::event::PlayerJoinEvent>([db, logger](ll::event::PlayerJoinEvent& ev) {
            logger->debug("PlayerJoinEvent: Player {} joining.", ev.self().getRealName());
            if (ev.self().isSimulatedPlayer()) {
                logger->debug("PlayerJoinEvent: Simulated player, skipping.");
                return;
            }
            if (!db->hasPlayerSettings(ev.self().getUuid().asString())) {
                logger->debug("PlayerJoinEvent: New player, creating settings for {}.", ev.self().getRealName());
                db->setPlayerSettings(ev.self().getUuid().asString(), PlayerSettings{}); // 新玩家
            }

            auto lands = db->getLands(ev.self().getXuid()); // xuid 查询
            if (!lands.empty()) {
                logger->info("PlayerJoinEvent: Found lands for player {} by XUID, attempting to update owner data.", ev.self().getRealName());
                auto uuid = ev.self().getUuid().asString();
                for (auto& land : lands) {
                    if (land->mIsConvertedLand && land->mOwnerDataIsXUID) {
                        logger->debug("PlayerJoinEvent: Updating land {} owner from XUID to UUID {}.", land->mID, uuid);
                        land->mLandOwner       = uuid; // xuid -> uuid
                        land->mOwnerDataIsXUID = false;
                    }
                }
            }
        }),
        bus->emplaceListener<ll::event::PlayerDisconnectEvent>([logger](ll::event::PlayerDisconnectEvent& ev) {
            auto& player = ev.self();
            if (player.isSimulatedPlayer()) {
                logger->debug("PlayerDisconnectEvent: Simulated player {} disconnecting, skipping.", player.getRealName());
                return;
            }
            logger->debug("PlayerDisconnectEvent: Player {} disconnect, remove all cache.", player.getRealName());

            auto& uuid    = player.getUuid();
            auto  uuidStr = uuid.asString();

            GlobalPlayerLocaleCodeCached.erase(uuidStr);
            LandScheduler::mDimidMap.erase(uuid);
            LandScheduler::mLandidMap.erase(uuid);
            SelectorManager::getInstance().cancel(player);
            DrawHandleManager::getInstance().removeHandle(player);
        }),
    };

    // LL
    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.ActorHurtEvent,
        bus->emplaceListener<ll::event::ActorHurtEvent>([db, logger](ll::event::ActorHurtEvent& ev) {
            auto& self   = ev.self();
            auto& source = ev.source();
            logger->debug(
                "ActorHurtEvent: Mob: {} ({}), ActorDamageCause: {}, SourceEntityType: {}",
                self.getTypeName(),
                self.getUniqueID().toString(),
                static_cast<int>(source.mCause),
                static_cast<int>(source.getEntityType())
            );

            auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
            if (PreCheck(land)) {
                logger->debug("ActorHurtEvent: No land or pre-check passed (operator/owner/member). Actor: {}", self.getTypeName());
                return;
            }
            logger->debug("ActorHurtEvent: Actor {} in land {}. Checking permissions.", self.getTypeName(), land->mID);

            if (source.getEntityType() == ActorType::Player
                && source.mCause == SharedTypes::Legacy::ActorDamageCause::EntityAttack) {
                logger->debug("ActorHurtEvent: Damage source is Player attacking.");
                if (auto souPlayer = self.getLevel().getPlayer(source.getEntityUniqueID()); souPlayer) {
                    if (PreCheck(land, souPlayer->getUuid().asString())) {
                        logger->debug("ActorHurtEvent: Attacking player {} is operator/owner/member. Allowed.", souPlayer->getRealName());
                        return;
                    }
                    logger->debug("ActorHurtEvent: Attacking player {} is Guest. Proceeding with permission check.", souPlayer->getRealName());
                }
            }

            if (land) {
                auto const& tab = land->getLandPermTableConst();
                if (!tab.allowAttackPlayer && self.isPlayer()) {
                    logger->debug("ActorHurtEvent: Player {} hurt, but allowAttackPlayer is false. Cancelling.", self.getTypeName());
                    ev.cancel();
                    return;
                }
                if (!tab.allowAttackAnimal && self.hasCategory(::ActorCategory::Animal)) {
                    logger->debug("ActorHurtEvent: Animal {} hurt, but allowAttackAnimal is false. Cancelling.", self.getTypeName());
                    ev.cancel();
                    return;
                }
                if (!tab.allowAttackMonster && self.hasCategory(::ActorCategory::Monster)) {
                    logger->debug("ActorHurtEvent: Monster {} hurt, but allowAttackMonster is false. Cancelling.", self.getTypeName());
                    ev.cancel();
                    return;
                }
                logger->debug("ActorHurtEvent: All attack checks passed for actor {}.", self.getTypeName());
            }
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.PlayerDestroyBlockEvent,
        bus->emplaceListener<ll::event::PlayerDestroyBlockEvent>([db, logger](ll::event::PlayerDestroyBlockEvent& ev) {
            auto& player   = ev.self();
            auto& blockPos = ev.pos();

            logger->debug("PlayerDestroyBlockEvent: Player {} destroying block at {}.", player.getRealName(), blockPos.toString());

            auto land = db->getLandAt(blockPos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                logger->debug("PlayerDestroyBlockEvent: No land or pre-check passed for player {}.", player.getRealName());
                return;
            }
            logger->debug("PlayerDestroyBlockEvent: Player {} destroying block in land {}. Checking permissions.", player.getRealName(), land->mID);

            auto& tab = land->getLandPermTableConst();
            if (tab.allowDestroy) {
                logger->debug("PlayerDestroyBlockEvent: allowDestroy is true. Allowed.");
                return;
            }

            logger->debug("PlayerDestroyBlockEvent: allowDestroy is false. Cancelling.");
            ev.cancel();
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.PlayerPlacingBlockEvent,
        bus->emplaceListener<ll::event::PlayerPlacingBlockEvent>([db, logger](ll::event::PlayerPlacingBlockEvent& ev) {
            auto&       player   = ev.self();
            auto const& blockPos = mc_utils::face2Pos(ev.pos(), ev.face()); // 计算实际放置位置

            logger->debug("PlayerPlacingBlockEvent: Player {} placing block at {}.", player.getRealName(), blockPos.toString());

            auto land = db->getLandAt(blockPos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                logger->debug("PlayerPlacingBlockEvent: No land or pre-check passed for player {}.", player.getRealName());
                return;
            }
            logger->debug("PlayerPlacingBlockEvent: Player {} placing block in land {}. Checking permissions.", player.getRealName(), land->mID);

            auto& tab = land->getLandPermTableConst();
            if (tab.allowPlace) {
                logger->debug("PlayerPlacingBlockEvent: allowPlace is true. Allowed.");
                return;
            }

            logger->debug("PlayerPlacingBlockEvent: allowPlace is false. Cancelling.");
            ev.cancel();
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.PlayerInteractBlockEvent,
        bus->emplaceListener<ll::event::PlayerInteractBlockEvent>([db,
                                                                   logger](ll::event::PlayerInteractBlockEvent& ev) {
            auto& player    = ev.self();
            auto& pos       = ev.blockPos();
            auto& itemStack = ev.item(); // 为了清晰，从 'item' 重命名为 'itemStack'
            auto  block     = ev.block().has_value() ? &ev.block().get() : nullptr;

            // 从 ItemStack 获取 Item 对象
            const Item* actualItem = itemStack.getItem(); // 假设 itemStack.getItem() 存在并返回 const Item*

            auto const  itemTypeNameForMap = itemStack.getTypeName(); // 用于 itemSpecificPermissionMap 的键
            auto const& blockTypeName      = block ? block->getTypeName() : "";

            logger->debug(
                "PlayerInteractBlockEvent: Player {} interacting at Pos: {}, item: {} (Item*: {}), block: {}",
                player.getRealName(),
                pos.toString(),
                itemTypeNameForMap,
                (void*)actualItem, // 记录 Item 指针以供调试
                blockTypeName
            );

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                logger->debug("PlayerInteractBlockEvent: No land or pre-check passed for player {}.", player.getRealName());
                return;
            }
            logger->debug("PlayerInteractBlockEvent: Player {} interacting in land {}. Checking permissions.", player.getRealName(), land->mID);

            auto const& tab = land->getLandPermTableConst();

            // --- 物品交互检查 ---
            logger->debug("PlayerInteractBlockEvent: --- Item Interaction Check ---");
            bool itemCancel = false;

            if (actualItem) {
                logger->debug("PlayerInteractBlockEvent: actualItem exists ({}). Checking vftable and specific map.", itemTypeNameForMap);
                void** itemVftable = *reinterpret_cast<void** const*>(actualItem);

                if ((itemVftable == BucketItem::$vftable() && !tab.useBucket)) {
                    logger->debug("PlayerInteractBlockEvent: Item is BucketItem, useBucket is {}. Cancel: {}", tab.useBucket, !tab.useBucket);
                    itemCancel = !tab.useBucket;
                } else if ((itemVftable == HatchetItem::$vftable() && !tab.allowAxePeeled)) {
                    logger->debug("PlayerInteractBlockEvent: Item is HatchetItem, allowAxePeeled is {}. Cancel: {}", tab.allowAxePeeled, !tab.allowAxePeeled);
                    itemCancel = !tab.allowAxePeeled;
                } else if ((itemVftable == HoeItem::$vftable() && !tab.useHoe)) {
                    logger->debug("PlayerInteractBlockEvent: Item is HoeItem, useHoe is {}. Cancel: {}", tab.useHoe, !tab.useHoe);
                    itemCancel = !tab.useHoe;
                } else if ((itemVftable == ShovelItem::$vftable() && !tab.useShovel)) {
                    logger->debug("PlayerInteractBlockEvent: Item is ShovelItem, useShovel is {}. Cancel: {}", tab.useShovel, !tab.useShovel);
                    itemCancel = !tab.useShovel;
                }
                
                if (!itemCancel) { // Only check specific map if not already cancelled by tool/bucket check
                    auto it = itemSpecificPermissionMap.find(itemTypeNameForMap);
                    if (it != itemSpecificPermissionMap.end()) {
                        bool perm = tab.*(it->second);
                        logger->debug("PlayerInteractBlockEvent: Item {} found in itemSpecificPermissionMap. Permission required: {}, Has: {}. Cancel: {}", itemTypeNameForMap, it->first, perm, !perm);
                        if (!perm) itemCancel = true;
                    } else {
                        logger->debug("PlayerInteractBlockEvent: Item {} not in itemSpecificPermissionMap.", itemTypeNameForMap);
                    }
                }
            } else {
                logger->debug("PlayerInteractBlockEvent: actualItem is null. Checking itemSpecificPermissionMap for item type name '{}'.", itemTypeNameForMap);
                auto it = itemSpecificPermissionMap.find(itemTypeNameForMap);
                if (it != itemSpecificPermissionMap.end()) {
                     bool perm = tab.*(it->second);
                    logger->debug("PlayerInteractBlockEvent: Item type name {} found in itemSpecificPermissionMap. Permission required: {}, Has: {}. Cancel: {}", itemTypeNameForMap, it->first, perm, !perm);
                    if (!perm) itemCancel = true;
                } else {
                     logger->debug("PlayerInteractBlockEvent: Item type name {} not in itemSpecificPermissionMap.", itemTypeNameForMap);
                }
            }

            if (itemCancel) {
                logger->debug("PlayerInteractBlockEvent: Item interaction check failed. Cancelling event.");
                ev.cancel();
                return;
            }
            logger->debug("PlayerInteractBlockEvent: Item interaction check passed.");

            // --- 方块交互检查 ---
            logger->debug("PlayerInteractBlockEvent: --- Block Interaction Check ---");
            if (block) {
                logger->debug("PlayerInteractBlockEvent: Block exists ({}).", blockTypeName);
                auto const& legacyBlock = block->getLegacyBlock();
                bool        blockCancel = false;

                auto blockIt = blockSpecificPermissionMap.find(blockTypeName);
            logger->debug("[AttackEntity] Entity: {}, Pos: {}", mob.getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            auto const& et  = mob.getTypeName();
            auto const& tab = land->getLandPermTableConst();
            if (et == "minecraft:ender_crystal" && tab.allowAttackEnderCrystal) return;      // 末影水晶
            if (et == "minecraft:armor_stand" && tab.allowDestroyArmorStand) return;         // 盔甲架
            if (tab.allowAttackPlayer && mob.isPlayer()) return;                             // 玩家

            bool isMonster = mob.hasCategory(::ActorCategory::Monster) || mob.hasFamily("monster");

            if (isMonster) {
                if (tab.allowAttackMonster) return;
            } else {
                if (tab.allowAttackAnimal) return;
            }

            ev.cancel();
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.PlayerPickUpItemEvent,
        bus->emplaceListener<ll::event::PlayerPickUpItemEvent>([db, logger](ll::event::PlayerPickUpItemEvent& ev) {
            auto& player = ev.self();
            auto& pos    = ev.itemActor().getPosition();

            logger->debug("[PickUpItem] Item: {}, Pos: {}", ev.itemActor().getTypeName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            if (land->getLandPermTableConst().allowPickupItem) return;

            ev.cancel();
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.SpawnedMobEvent,
        bus->emplaceListener<ll::event::SpawnedMobEvent>([db, logger](ll::event::SpawnedMobEvent& ev) {
            auto mob = ev.mob();
            if (!mob.has_value()) {
                return;
            }

            auto& pos = mob->getPosition();

            logger->debug("[SpawnedMob] {}", pos.toString());

            auto land = db->getLandAt(pos, mob->getDimensionId());
            if (PreCheck(land)) {
                return;
            }

            auto const& tab = land->getLandPermTableConst();
            bool isMonster = mob->hasCategory(::ActorCategory::Monster) || mob->hasFamily("monster");

            if (isMonster) {
                if (!tab.allowMonsterSpawn) {
                    mob->despawn();
                }
            } else {
                if (!tab.allowAnimalSpawn) {
                    mob->despawn();
                }
            }
        })
    )

    // ila
    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.PlayerAttackBlockBeforeEvent,
        bus->emplaceListener<ila::mc::PlayerAttackBlockBeforeEvent>(
            [db, logger](ila::mc::PlayerAttackBlockBeforeEvent& ev) {
                auto& self = ev.self();
                auto& pos  = ev.pos();

                logger->debug("[AttackBlock] {}", pos.toString());

                auto land = db->getLandAt(pos, self.getDimensionId());
                if (PreCheck(land, self.getUuid().asString())) return; // land not found

                auto const& blockTypeName = self.getDimensionBlockSourceConst().getBlock(pos).getTypeName();
                CANCEL_AND_RETURN_IF(
                    !land->getLandPermTableConst().allowAttackDragonEgg && blockTypeName == "minecraft:dragon_egg"
                );
            }
        )
    );

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.ArmorStandSwapItemBeforeEvent,
        bus->emplaceListener<ila::mc::ArmorStandSwapItemBeforeEvent>(
            [db, logger](ila::mc::ArmorStandSwapItemBeforeEvent& ev) {
                Player& player = ev.player();

                logger->debug("[ArmorStandSwapItem]: executed");

                auto land = db->getLandAt(ev.self().getPosition(), player.getDimensionId());
                if (PreCheck(land, player.getUuid().asString())) {
                    return;
                }

                if (land->getLandPermTableConst().useArmorStand) return;

                ev.cancel();
            }
        )
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.PlayerDropItemBeforeEvent,
        bus->emplaceListener<ila::mc::PlayerDropItemBeforeEvent>([db, logger](ila::mc::PlayerDropItemBeforeEvent& ev) {
            Player& player = ev.self();

            logger->debug("[PlayerDropItem]: executed");

            auto land = db->getLandAt(player.getPosition(), player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            if (land->getLandPermTableConst().allowDropItem) return;

            ev.cancel();
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.ActorRideBeforeEvent,
        bus->emplaceListener<ila::mc::ActorRideBeforeEvent>([db, logger](ila::mc::ActorRideBeforeEvent& ev) {
            logger->debug("[ActorRide]: executed");
            Actor& passenger = ev.self();

            if (!passenger.isPlayer()) {
                return; // 忽略非玩家骑乘事件
            }

            auto const& typeName = ev.target().getTypeName();
            auto        land     = db->getLandAt(passenger.getPosition(), passenger.getDimensionId());
            if (PreCheck(land)) return; // land not found
            // 特殊处理：
            if (land) {
                auto& tab = land->getLandPermTableConst();
                if (typeName == "minecraft:minecart" || typeName == "minecraft:boat") {
                    if (tab.allowRideTrans) return;
                } else {
                    if (tab.allowRideEntity) return;
                }
            }

            if (passenger.isPlayer()) {
                auto player = passenger.getWeakEntity().tryUnwrap<Player>();
                if (player.has_value() && PreCheck(land, player->getUuid().asString())) {
                    return;
                }
            }

            ev.cancel();
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.ExplosionBeforeEvent,
        bus->emplaceListener<ila::mc::ExplosionBeforeEvent>([db, logger](ila::mc::ExplosionBeforeEvent& ev) {
            logger->debug("[Explode] Pos: {}", ev.explosion().mPos->toString());

            auto lands = db->getLandAt(
                BlockPos{ev.explosion().mPos},
                (int)(ev.explosion().mRadius + 1.0),
                ev.blockSource().getDimensionId()
            );
            for (auto& p : lands) {
                if (!p->getLandPermTableConst().allowExplode) {
                    ev.cancel();
                    break;
                }
            }
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.FarmDecayBeforeEvent,
        bus->emplaceListener<ila::mc::FarmDecayBeforeEvent>([db, logger](ila::mc::FarmDecayBeforeEvent& ev) {
            logger->debug("[FarmDecay] Pos: {}", ev.pos().toString());

            auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
            if (PreCheck(land)) return; // land not found
            if (land) {
                if (land->getLandPermTableConst().allowFarmDecay) return;
            }

            ev.cancel();
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.MobHurtEffectBeforeEvent,
        bus->emplaceListener<ila::mc::MobHurtEffectBeforeEvent>([db, logger](ila::mc::MobHurtEffectBeforeEvent& ev) {
            logger->debug("[MobHurtEffect] mob: {}", ev.self().getTypeName());
            auto& self = ev.self();

            auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
            if (PreCheck(land)) return; // land not found
            if (land) {
                auto const& tab = land->getLandPermTableConst();
                if (tab.allowAttackPlayer && self.isPlayer()) return;
                if (tab.allowAttackAnimal && self.hasCategory(::ActorCategory::Animal)) return;
                if (tab.allowAttackMonster && self.hasCategory(::ActorCategory::Monster)) return;
            }

            if (self.isPlayer()) {
                auto const pl = self.getWeakEntity().tryUnwrap<Player>();
                if (pl.has_value()) {
                    if (PreCheck(land, pl->getUuid().asString())) return;
                }
            }

            ev.cancel();
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.PistonPushBeforeEvent,
        bus->emplaceListener<ila::mc::PistonPushBeforeEvent>([db, logger](ila::mc::PistonPushBeforeEvent& ev) {
            auto const& piston = ev.pistonPos();
            auto const& push   = ev.pushPos();
            auto&       region = ev.blockSource();

            logger->debug("[PistonTryPush] piston: {}, push: {}", piston.toString(), push.toString());

            auto land  = db->getLandAt(push, region.getDimensionId());
            auto land2 = db->getLandAt(piston, region.getDimensionId());
            if (land && !land->getLandPermTableConst().allowPistonPush && land != land2) {
                ev.cancel();
            }
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.PlayerOperatedItemFrameBeforeEvent,
        bus->emplaceListener<ila::mc::PlayerOperatedItemFrameBeforeEvent>(
            [db, logger](ila::mc::PlayerOperatedItemFrameBeforeEvent& ev) {
                logger->debug("[PlayerUseItemFrame] pos: {}", ev.blockPos().toString());

                auto land = db->getLandAt(ev.blockPos(), ev.self().getDimensionId());
                if (PreCheck(land, ev.self().getUuid().asString())) return;

                if (land->getLandPermTableConst().useItemFrame) return;

                ev.cancel();
            }
        )
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.ActorTriggerPressurePlateBeforeEvent,
        bus->emplaceListener<ila::mc::ActorTriggerPressurePlateBeforeEvent>(
            [db, logger](ila::mc::ActorTriggerPressurePlateBeforeEvent& ev) {
                logger->debug("[PressurePlateTrigger] pos: {}", ev.pos().toString());

                auto land = db->getLandAt(ev.pos(), ev.self().getDimensionId());
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
            }
        )
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.ProjectileCreateBeforeEvent,
        bus->emplaceListener<ila::mc::ProjectileCreateBeforeEvent>([db,
                                                                    logger](ila::mc::ProjectileCreateBeforeEvent& ev) {
            Actor& self = ev.self();
            auto&  type = ev.self().getTypeName();

            logger->debug("[ProjectileSpawn] type: {}", type);

            auto land = db->getLandAt(self.getPosition(), self.getDimensionId());
            if (PreCheck(land)) return; // land not found

            if (self.getOwnerEntityType() == ActorType::Player) {
                // 由玩家所创建的实体
                if (auto mob = self.getOwner(); mob && mob->isPlayer()) {
                    auto pl = mob->getWeakEntity().tryUnwrap<Player>();
                    if (pl.has_value() && PreCheck(land, pl->getUuid().asString())) return;
                }
            }

            if (land) {
                auto const& tab = land->getLandPermTableConst();
                auto it = projectilePermissionMap.find(type); 
                if (it != projectilePermissionMap.end()) {
                    CANCEL_AND_RETURN_IF(!(tab.*(it->second))); 
                }
            }
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.RedstoneUpdateBeforeEvent,
        bus->emplaceListener<ila::mc::RedstoneUpdateBeforeEvent>([db, logger](ila::mc::RedstoneUpdateBeforeEvent& ev) {
            // logger->debug("[RedstoneUpdate] Pos: {}", ev.Pos().toString());

            auto land = db->getLandAt(ev.pos(), ev.blockSource().getDimensionId());
            if (PreCheck(land)) return; // land not found
            if (land) {
                if (land->getLandPermTableConst().allowRedstoneUpdate) return;
            }

            ev.cancel();
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.WitherDestroyBeforeEvent,
        bus->emplaceListener<ila::mc::WitherDestroyBeforeEvent>([db, logger](ila::mc::WitherDestroyBeforeEvent& ev) {
            logger->debug("[WitherDestroyBlock] executed");
            auto& aabb = ev.box();

            auto lands = db->getLandAt(aabb.min, aabb.max, ev.blockSource().getDimensionId());
            for (auto const& p : lands) {
                if (!p->getLandPermTableConst().allowWitherDestroy) {
                    ev.cancel();
                    break;
                }
            }
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.MossGrowthBeforeEvent,
        bus->emplaceListener<ila::mc::MossGrowthBeforeEvent>([db, logger](ila::mc::MossGrowthBeforeEvent& ev) {
            // logger->debug("[MossSpread] {}", ev.Pos().toString());

            auto const& pos = ev.pos();

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
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.LiquidTryFlowBeforeEvent,
        bus->emplaceListener<ila::mc::LiquidTryFlowBeforeEvent>([db, logger](ila::mc::LiquidTryFlowBeforeEvent& ev) {
            auto& sou = ev.pos();
            // auto& from = ev.FlowFromPos();
            // logger->debug("[LiquidFlow] {} -> {}", sou.toString(), from.toString());

            auto land = db->getLandAt(sou, ev.blockSource().getDimensionId());
            if (land && !land->getLandPermTableConst().allowLiquidFlow) {
                ev.cancel();
            }
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.SculkBlockGrowthBeforeEvent,
        bus->emplaceListener<ila::mc::SculkBlockGrowthBeforeEvent>([db,
                                                                    logger](ila::mc::SculkBlockGrowthBeforeEvent& ev) {
            auto& pos = ev.pos();
            logger->debug("[SculkBlockGrowth] {}", pos.toString());

            auto land = db->getLandAt(pos, ev.blockSource().getDimensionId());
            if (land) {
                if (!land->getLandPermTableConst().allowSculkBlockGrowth) {
                    ev.cancel();
                }
            }
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.SculkSpreadBeforeEvent,
        bus->emplaceListener<ila::mc::SculkSpreadBeforeEvent>([db, logger](ila::mc::SculkSpreadBeforeEvent& ev) {
            // logger->debug("[SculkSpread] {} -> {}", ev.SelfPos().toString(), ev.TargetPos().toString());

            auto sou = db->getLandAt(ev.selfPos(), ev.blockSource().getDimensionId());
            auto tar = db->getLandAt(ev.targetPos(), ev.blockSource().getDimensionId());

            if (!sou && !tar) return; // 领地外蔓延
            if (sou && tar) return;   // 领地内蔓延
            if (sou && !tar) return;  // 领地内蔓延到外
            if (!sou && tar) {
                ev.cancel(); // 外蔓延到领地内
            }
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.PlayerEditSignBeforeEvent,
        bus->emplaceListener<ila::mc::PlayerEditSignBeforeEvent>([db, logger](ila::mc::PlayerEditSignBeforeEvent& ev) {
            auto& player = ev.self();
            auto& pos    = ev.pos();

            logger->debug("[PlayerEditSign] {} -> {}", player.getRealName(), pos.toString());

            auto land = db->getLandAt(pos, player.getDimensionId());
            if (PreCheck(land, player.getUuid().asString())) {
                return;
            }

            if (land && !land->getLandPermTableConst().editSign) {
                ev.cancel();
            }
        })
    )

    CHECK_EVENT_AND_REGISTER_LISTENER(
        Config::cfg.listeners.SculkCatalystAbsorbExperienceBeforeEvent,
        bus->emplaceListener<ila::mc::SculkCatalystAbsorbExperienceBeforeEvent>(
            [db, logger](ila::mc::SculkCatalystAbsorbExperienceBeforeEvent& ev) {
                // auto& actor  = ev.DiedActor();
                auto& actor  = ev.actor();
                auto& region = actor.getDimensionBlockSource();
                auto  pos    = actor.getBlockPosCurrentlyStandingOn(&actor);
                logger->debug("[SculkCatalystAbsorbExperience] Pos: {}", pos.toString());
                // 领地内蔓延 && 半径内没有别的领地 => 放行
                // 领地外蔓延 && 半径内有别的领地   => 放行
                auto cur = db->getLandAt(pos, region.getDimensionId());
                auto lds = db->getLandAt(pos - 9, pos + 9, region.getDimensionId());
                if (cur && lds.size() == 1) return;
                if (!cur && lds.empty()) return;
                ev.cancel();
            }
        )
    )


    return true;
}
#undef CHECK_EVENT_AND_REGISTER_LISTENER
#undef CANCEL_AND_RETURN_IF


bool EventListener::release() {
    auto& bus = ll::event::EventBus::getInstance();

    for (auto& l : mListeners) {
        bus.removeListener(l);
    }

    return true;
}




} // namespace land
