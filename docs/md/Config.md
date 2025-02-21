# 配置文件

> 配置文件为`json`格式，位于`plugins/PLand/`目录下，文件名为`config.json`。

!> 配置文件为`json`格式，请勿使用记事本等不支持`json`格式的文本编辑器进行编辑，否则会导致配置文件损坏。

## 配置文件结构

```json
{
  "version": 2, // 配置文件版本，请勿修改
  "logLevel": "Info", // 日志等级 Off / Fatal / Error / Warn / Info / Debug / Trace
  "economy": {
    "enabled": true, // 是否启用经济系统
    "kit": "LegacyMoney", // 经济套件 LegacyMoney 或 ScoreBoard
    "economyName": "money", // 经济名称
    "scoreboardObjName": "" // 计分板对象名称
  },
  "land": {
    "landTp": true, // 是否启用领地传送
    "maxLand": 20, // 玩家最大领地数量
    "minSpacing": 16, // 领地最小间距
    "refundRate": 0.9, // 退款率(0.0~1.0，1.0为全额退款，0.9为退还90%)
    "discountRate": 1.0, // 折扣率(0.0~1.0，1.0为原价，0.9为打9折)

    "setupDrawCommand": true, // 是否注册领地范围绘制指令
    "drawRange": 64, // 绘制查询领地范围 （以玩家为中心，此值不宜过大，过大可能导致性能问题）

    "tip": {
      "enterTip": true, // 是否启用进入领地提示
      "bottomContinuedTip": true, // 是否启用领地底部持续提示
      "bottomTipFrequency": 1 // 领地底部持续提示频率(s)
    },
    "bought": {
      "threeDimensionl": {
        "enabled": true, // 是否启用三维领地(购买)
        "calculate": "square * 25" // 领地价格计算公式
      },
      "twoDimensionl": {
        "enabled": true, // 是否启用二维领地(购买)
        "calculate": "square * 8 + height * 20" // 领地价格计算公式
      },
      "squareRange": {
        "min": 4, // 领地最小面积
        "max": 60000, // 领地最大面积
        "minHeight": 1 // 领地最小高度
      },
      "allowDimensions": [
        // 允许圈地的维度
        0, 1, 2
      ]
    }
  },
  "selector": {
    "drawParticle": true, // 是否启用绘制粒子
    "particle": "minecraft:villager_happy", // 粒子类型
    "tool": "minecraft:stick" // 选择工具
  },
  "listeners": {
    // 事件监听器开关 true 为开启，false 为关闭
    // 注意：非必要情况下，请勿关闭事件监听器，否则可能导致领地功能异常
    // 如果您不知道某个事件监听器是做什么的，请不要关闭它
    // 提供的注释仅概括了大致作用，具体行为请查看源码
    "ActorHurtEvent": true, // 实体受伤事件
    "PlayerDestroyBlockEvent": true, // 玩家破坏方块事件
    "PlayerPlacingBlockEvent": true, // 玩家放置方块事件
    "PlayerInteractBlockEvent": true, // 玩家交互方块事件(使用物品)
    "FireSpreadEvent": true, // 火势蔓延事件
    "PlayerAttackEvent": true, // 玩家攻击事件
    "PlayerPickUpItemEvent": true, // 玩家拾取物品事件
    "PlayerInteractBlockEvent1": true, // 玩家交互方块事件(功能性方块)
    "PlayerUseItemEvent": true, // 玩家使用物品事件
    "PlayerAttackBlockBeforeEvent": true, // 玩家攻击方块事件
    "ArmorStandSwapItemBeforeEvent": true, // 盔甲架交换物品事件
    "PlayerDropItemBeforeEvent": true, // 玩家丢弃物品事件
    "ActorRideBeforeEvent": true, // 实体骑乘事件
    "ExplosionBeforeEvent": true, // 爆炸事件
    "FarmDecayBeforeEvent": true, // 农田枯萎事件
    "MobHurtEffectBeforeEvent": true, // 生物受伤效果事件
    "PistonPushBeforeEvent": true, // 活塞推动事件
    "PlayerOperatedItemFrameBeforeEvent": true, // 玩家操作物品展示框事件
    "ActorTriggerPressurePlateBeforeEvent": true, // 实体触发压力板事件
    "ProjectileCreateBeforeEvent": true, // 投掷物创建事件
    "RedstoneUpdateBeforeEvent": true, // 红石更新事件
    "WitherDestroyBeforeEvent": true, // 凋零破坏事件
    "MossGrowthBeforeEvent": true, // 苔藓生长事件
    "LiquidTryFlowBeforeEvent": true, // 液体尝试流动事件
    "SculkBlockGrowthBeforeEvent": true, // 诡秘方块生长事件
    "SculkSpreadBeforeEvent": true, // 诡秘蔓延事件
    "PlayerEditSignBeforeEvent": true, // 玩家编辑告示牌事件
    "SpawnedMobEvent": true, // 生物生成事件(怪物和动物)
    "SculkCatalystAbsorbExperienceBeforeEvent": false // 幽匿催化体吸收经验事件
  },
  "internal": {
    "devTools": false // 是否启用开发工具，启用前请确保您的机器有具有显示器，否则初始化时会引发错误、甚至崩溃。
  }
}
```

## calculate 计算公式

?> PLand 的 `Calculate` 实现使用了 [`exprtk`](https://github.com/ArashPartow/exprtk) 库，因此你可以使用 [`exprtk`](https://github.com/ArashPartow/exprtk) 库所支持的所有函数和运算符。

|   变量   |     描述     |
| :------: | :----------: |
| `height` |   领地高度   |
| `width`  |   领地宽度   |
| `depth`  | 领地深度(长) |
| `square` |   领地面积   |
| `volume` |   领地体积   |
