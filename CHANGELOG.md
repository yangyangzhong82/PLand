# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.3.1] - 2024-12-17

### Fixed

- 修复 `ActorHurtEvent` `ActorRideEvent` `FarmDecayEvent` `MobHurtEffectEvent` `PressurePlateTriggerEvent` `ProjectileSpawnEvent` `RedstoneUpdateEvent` 事件意外拦截领地外事件。

## [0.3.0] - 2024-12-15

### Added

- 支持从 iLand 导入数据
- LandData 新增 `mIsConvertedLand`、`mOwnerDataIsXUID` 字段
- 新增 `ActorHurtEvent` 事件处理

## [0.2.7] - 2024-12-6

### Fixed

- 修复删除领地时出现回档问题 [#5]

## [0.2.6] - 2024-11-17

### Changed

- `PLand` 部分成员更改为 `private`

### Fixed

- 修复 `PLand::generateLandID` 可能生成重复的 ID
- 修复 `PLand::addLand` 添加失败依然返回 true
- 修复 `LandBuyGui::impl` 购买失败未返还经济

## [0.2.5] - 2024-11-15

### Fixed

- 修复可能的死锁问题
- 修复 PLand 多线程竞争问题

## [0.2.4] - 2024-11-13

### Changed

- `IChoosePlayerFromDB` 表单增加去重
- `PLand` 增加 `std::mutex` 保护资源

## [0.2.3] - 2024-11-2

### Changed

- `/pland op <target: Player>` 添加未找到玩家时错误提示
- `/pland buy` 领地范围不合法提示添加当前范围信息

### Fixed

- 修复 `LandPos::getSquare`、`LandPos::getVolume` 计算错误

## [0.2.2] - 2024-11-1

### Fixed

- 修复购买 2D 领地时范围验证错误

## [0.2.1] - 2024-10-31

### Fixed

- 修复领地绘制引发的崩溃

## [0.2.0] - 2024-10-16

### Added

- 新增领地绘制功能

## [0.1.0] - 2024-9-27

### Added

- 新增 MossSpreadEvent 事件处理
- 新增 PistonTryPushEvent 事件处理
- 新增 LiquidFlowEvent 事件处理
- 新增 SculkCatalystAbsorbExperienceEvent 事件处理
- 新增 allowLiquidFlow 权限

## [0.0.3] - 2024-9-21

### Added

- 完成权限翻译

### Changed

- 购买领地后，立即清除标题
- 修改 3D 选区时调整 Y 轴 GUI 错误提示

### Fixed

- 修复编辑领地成员返回按钮异常
- 修复领地购买范围异常

## [0.0.2] - 2024-9-21

### Added

- 新增 12 个事件处理
- 新增依赖 MoreEvents

## [0.0.1] - 2024-9-18

- Initial beta release
