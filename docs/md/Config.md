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
            "allowDimensions": [ // 允许圈地的维度
                0,
                1,
                2
            ]
        }
    },
    "selector": {
        "drawParticle": true, // 是否启用绘制粒子
        "particle": "minecraft:villager_happy", // 粒子类型
        "tool": "minecraft:stick" // 选择工具
    }
}
```

## calculate 计算公式

?> PLand 的 `Calculate` 实现使用了 `exprtk` 库，因此你可以使用 `exprtk` 库所支持的所有函数和运算符。

| 变量 | 描述 |
| :---: | :---: |
| `height` | 领地高度 |
| `width` | 领地宽度 |
| `depth` | 领地深度(长) |
| `square` | 领地面积 |
| `volume` | 领地体积 |
