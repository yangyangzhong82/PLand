# 命令列表

插件提供以下命令进行简单的交互：

```log
? pland
23:01:00.561 INFO [Server] pland:
23:01:00.561 INFO [Server] PLand 领地系统
23:01:00.561 INFO [Server] Usage:
23:01:00.561 INFO [Server] - /pland
23:01:00.561 INFO [Server] - /pland <deop|op> <player: target>
23:01:00.561 INFO [Server] - /pland buy
23:01:00.561 INFO [Server] - /pland cancel
23:01:00.561 INFO [Server] - /pland gui
23:01:00.561 INFO [Server] - /pland mgr
23:01:00.561 INFO [Server] - /pland new
23:01:00.561 INFO [Server] - /pland reload
23:01:00.561 INFO [Server] - /pland set <a|b>
```

?> 其中`pland` 为插件的顶层命令

- `/pland <op/deop> <player: target>`
  - 添加或移除目标玩家的领地管理员权限。

- `pland new`
  - 创建领地。

- `/pland cancel`
  - 取消领地创建 (需要先执行 `/pland new` 命令)。

- `/pland set <a|b>`
  - 设置领地位置 (需要先执行 `/pland new` 命令)。

- `/pland buy`
  - 购买领地 (需要先执行 `/pland new` 命令)。

- `/pland gui`
  - 打开领地 GUI。

- `/pland mgr`
  - 打开领地管理 GUI (领地管理员)。

- `/pland reload`
  - 重载领地配置 (控制台)。

- `/pland draw <disable|current|near>`
  - 开启绘制领地范围(需在 `Config.json` 中设置 `setupDrawCommand: true`)
    - `disable` 关闭绘制
    - `current` 绘制当前所在领地范围
    - `near` 绘制附近领地范围（范围由 `Config.json` 中的 `drawRange` 设置）
