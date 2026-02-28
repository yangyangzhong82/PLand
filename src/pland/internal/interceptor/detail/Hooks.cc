#include "pland/internal/interceptor/EventInterceptor.h"
#include "pland/internal/interceptor/InterceptorConfig.h"
#include "pland/internal/interceptor/helper/InterceptorHelper.h"

#include "pland/PLand.h"
#include "pland/land/repo/LandRegistry.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/entity/ActorHurtEvent.h"
#include "ll/api/memory/Hook.h"

#include "mc/server/ServerPlayer.h"
#include "mc/world/actor/ActorDamageSource.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/FishingHook.h"
#include "mc/world/actor/Mob.h"
#include "mc/world/actor/ai/goal/LayEggGoal.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/FireBlock.h"
#include "mc/world/level/block/actor/ChestBlockActor.h"

#include "mc/entity/components_json_legacy/HopperComponent.h"
#include "mc/world/actor/global/LightningBolt.h"
#include "mc/world/effect/OozingMobEffect.h"
#include "mc/world/effect/WeavingMobEffect.h"
#include "mc/world/level/block/LecternBlock.h"
#include "mc/world/level/block/block_events/BlockPlayerInteractEvent.h"

namespace land::internal::interceptor {

// Fix [#140](https://github.com/engsr6982/PLand/issues/140)
// 部分实体受伤情况不走LL的ActorHurtEvent，需要在子类Mob受伤逻辑处拦截
LL_TYPE_INSTANCE_HOOK(
    MobHurtHook,
    HookPriority::Normal,
    Mob,
    &Mob::$_hurt,
    bool,
    ::ActorDamageSource const& source,
    float                      damage,
    bool                       knock,
    bool                       ignite
) {
    if (source.getEntityType() != ActorType::Player) {
        return origin(source, damage, knock, ignite);
    }

    auto player = this->getLevel().getPlayer(source.getEntityUniqueID());
    if (!player) {
        return origin(source, damage, knock, ignite);
    }

    auto& registry = PLand::getInstance().getLandRegistry();
    auto& actor    = *this;
    auto& uuid     = player->getUuid();
    auto  land     = registry.getLandAt(actor.getPosition(), actor.getDimensionId());
    if (hasPrivilege(land, uuid)) {
        return origin(source, damage, knock, ignite);
    }

    if (actor.isPlayer()) {
        if (!hasMemberOrGuestPermission<&RolePerms::allowPvP>(land, uuid)) {
            return false;
        }
    }

    HashedString typeName{actor.getTypeName()};
    if (InterceptorConfig::cfg.rules.mob.allowFriendlyDamage.contains(typeName)) {
        if (!hasMemberOrGuestPermission<&RolePerms::allowFriendlyDamage>(land, uuid)) {
            return false;
        }
    } else if (InterceptorConfig::cfg.rules.mob.allowHostileDamage.contains(typeName)) {
        if (!hasMemberOrGuestPermission<&RolePerms::allowHostileDamage>(land, uuid)) {
            return false;
        }
    } else if (InterceptorConfig::cfg.rules.mob.allowSpecialEntityDamage.contains(typeName)) {
        if (!hasMemberOrGuestPermission<&RolePerms::allowSpecialEntityDamage>(land, uuid)) {
            return false;
        }
    }

    return origin(source, damage, knock, ignite);
}
// Fix [#56](https://github.com/engsr6982/PLand/issues/56)
LL_TYPE_INSTANCE_HOOK(
    FishingHookHitHook,
    ll::memory::HookPriority::Normal,
    ::FishingHook,
    &::FishingHook::_pullCloser,
    void,
    Actor& inEntity,
    float  inSpeed
) {
    // 获取钓鱼钩的位置和维度
    auto& hookActor = *this;
    auto* player    = hookActor.getPlayerOwner();
    if (!player) {
        origin(inEntity, inSpeed);
        return;
    }

    auto& pos   = hookActor.getPosition();
    auto  dimId = hookActor.getDimensionId();

    auto& db   = PLand::getInstance().getLandRegistry();
    auto  land = db.getLandAt(pos, dimId);
    if (!hasRolePermission<&RolePerms::allowFishingRodAndHook>(land, player->getUuid())) {
        return;
    }

    origin(inEntity, inSpeed);
}

// Fix [#69](https://github.com/engsr6982/PLand/issues/69)
LL_TYPE_INSTANCE_HOOK(
    LayEggGoalHook,
    ll::memory::HookPriority::Normal,
    ::LayEggGoal,
    &::LayEggGoal::$isValidTarget,
    bool,
    ::BlockSource&    region,
    ::BlockPos const& pos
) {
    auto& db   = PLand::getInstance().getLandRegistry();
    auto  land = db.getLandAt(pos, region.getDimensionId());
    if (!hasEnvironmentPermission<&EnvironmentPerms::allowMobGrief>(land)) {
        return false; // 如果领地内不允许实体破坏，则阻止产蛋
    }
    return origin(region, pos);
}

// Fix [#136](https://github.com/engsr6982/PLand/issues/136)
LL_TYPE_INSTANCE_HOOK(
    FireBlockBurnHook,
    ll::memory::HookPriority::Normal,
    FireBlock,
    &FireBlock::checkBurn,
    void,
    ::BlockSource&    region,
    ::BlockPos const& pos,
    int               chance,
    ::Randomize&      randomize,
    int               age,
    ::BlockPos const& firePos
) {
    auto& db   = PLand::getInstance().getLandRegistry();
    auto  land = db.getLandAt(pos, region.getDimensionId());
    if (!hasEnvironmentPermission<&EnvironmentPerms::allowFireSpread>(land)) {
        return; // 如果领地内不允许火焰蔓延，则阻止蔓延
    }
    origin(region, pos, chance, randomize, age, firePos);
}


// Fix [#158](https://github.com/engsr6982/PLand/issues/158)
LL_TYPE_INSTANCE_HOOK(
    ChestBlockActorOpenHook,
    ll::memory::HookPriority::Normal,
    ChestBlockActor,
    &ChestBlockActor::$startOpen,
    void,
    ::Actor& actor
) {
    if (actor.isPlayer()) {
        origin(actor);
        return;
    }
    auto& db   = PLand::getInstance().getLandRegistry();
    auto  land = db.getLandAt(this->mPosition, actor.getDimensionId());
    if (!hasGuestPermission<&RolePerms::useContainer>(land)) {
        return; // 访客权限不允许，拦截铜傀儡开箱子
    }
    origin(actor);
}

// fix: [#167](https://github.com/IceBlcokMC/PLand/issues/167)
LL_TYPE_INSTANCE_HOOK(
    LightningBoltHook,
    ll::memory::HookPriority::Normal,
    LightningBolt,
    &LightningBolt::$normalTick,
    void
) {
    auto& registry = PLand::getInstance().getLandRegistry();
    if (auto land = registry.getLandAt(this->getPosition(), this->getDimensionId())) {
        if (!hasEnvironmentPermission<&EnvironmentPerms::allowLightningBolt>(land)) {
            this->remove(); // 必须标记移除，否则闪电实体不会被移除且会一直tick
            return;         // 不允许闪电，拦截 tick
        }
    }
    origin();
}

// Fix [#143](https://github.com/IceBlcokMC/PLand/issues/143)
LL_TYPE_INSTANCE_HOOK(
    LecternBlockUseHook,
    ll::memory::HookPriority::Normal,
    LecternBlock,
    &LecternBlock::use,
    void,
    BlockEvents::BlockPlayerInteractEvent& event
) {
    auto& player = event.mPlayer;
    auto& pos    = event.mPos;

    auto& registry = PLand::getInstance().getLandRegistry();
    auto  land     = registry.getLandAt(pos, player.getDimensionId());
    if (!hasRolePermission<&RolePerms::useLectern>(land, player.getUuid())) {
        return; // 拦截阅读/放置书本
    }
    origin(event);
}
LL_TYPE_INSTANCE_HOOK(
    LecternBlockDropBookHook,
    ll::memory::HookPriority::Normal,
    LecternBlock,
    &LecternBlock::_dropBook,
    bool,
    Player&         player,
    BlockPos const& pos
) {
    auto& registry = PLand::getInstance().getLandRegistry();
    auto  land     = registry.getLandAt(pos, player.getDimensionId());
    if (!hasRolePermission<&RolePerms::useLectern>(land, player.getUuid())) {
        return false; // 拦截取下书本
    }
    return origin(player, pos);
}

// Fix [#59](https://github.com/IceBlcokMC/PLand/issues/59)
LL_TYPE_INSTANCE_HOOK(
    OozingMobEffectHook,
    ll::memory::HookPriority::Normal,
    OozingMobEffect,
    &OozingMobEffect::$onActorDied,
    void,
    ::Actor& actor,
    int      amplifier
) {
    // Wiki: 此效果的生物死亡时，会尝试在死亡处生成2只中型史莱姆
    auto& pos      = actor.getPosition();
    auto& registry = PLand::getInstance().getLandRegistry();
    auto  land     = registry.getLandAt(pos, actor.getDimensionId());
    if (!hasEnvironmentPermission<&EnvironmentPerms::allowMonsterSpawn>(land)) {
        return;
    }
    origin(actor, amplifier);
}
LL_TYPE_INSTANCE_HOOK(
    WeavingMobEffectHook,
    ll::memory::HookPriority::Normal,
    WeavingMobEffect,
    &WeavingMobEffect::$onActorDied,
    void,
    ::Actor& actor,
    int      amplifier
) {
    // Wiki: 当游戏规则mobGriefing为true时，拥有盘丝的生物死亡后会在以自身为中心3×3×3的范围内尝试生成2-3个蜘蛛网
    auto& pos      = actor.getPosition();
    auto& registry = PLand::getInstance().getLandRegistry();
    auto  land     = registry.getLandAt(pos, actor.getDimensionId());
    if (!hasEnvironmentPermission<&EnvironmentPerms::allowMobGrief>(land)) {
        return;
    }
    origin(actor, amplifier);
}


// https://github.com/IceBlcokMC/PLand/issues/55
LL_TYPE_INSTANCE_HOOK(
    HopperComponentPullInItemsHook,
    ll::memory::HookPriority::Normal,
    HopperComponent,
    &HopperComponent::pullInItems,
    bool,
    ::Actor& owner // 拥有此组件的 Actor
) {
    if (!owner.isType(ActorType::MinecartHopper)) {
        return origin(owner);
    }

    auto& registry = PLand::getInstance().getLandRegistry();
    auto  land     = registry.getLandAt(owner.getPosition(), owner.getDimensionId());
    if (!hasEnvironmentPermission<&EnvironmentPerms::allowMinecartHopperPullItems>(land)) {
        return false;
    }
    return origin(owner);
}


void EventInterceptor::setupHooks() {
    auto& config = InterceptorConfig::cfg.hooks;
    registerHookIf<MobHurtHook>(config.MobHurtHook);
    registerHookIf<FishingHookHitHook>(config.FishingHookHitHook);
    registerHookIf<LayEggGoalHook>(config.LayEggGoalHook);
    registerHookIf<FireBlockBurnHook>(config.FireBlockBurnHook);
    registerHookIf<ChestBlockActorOpenHook>(config.ChestBlockActorOpenHook);
    registerHookIf<LightningBoltHook>(config.LightningBoltHook);
    registerHookIf<LecternBlockUseHook>(config.LecternBlockUseHook);
    registerHookIf<LecternBlockDropBookHook>(config.LecternBlockDropBookHook);
    registerHookIf<OozingMobEffectHook>(config.OozingMobEffectHook);
    registerHookIf<WeavingMobEffectHook>(config.WeavingMobEffectHook);
    registerHookIf<HopperComponentPullInItemsHook>(config.HopperComponentPullInItemsHook);
}

} // namespace land::internal::interceptor