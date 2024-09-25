# Event & 领地事件

?> 领地的事件分为 `Before` 和 `After` 两个阶段，`Before` 阶段为事件触发前，`After` 阶段为事件触发后。

?> `Before` 事件可以通过 `ev.cancel()` 取消事件，`After` 事件无法取消。

?> 命名规则: `事件名` + `Before` / `After` + `Event`  
例如: `PlayerAskCreateLandBeforeEvent`

## 事件列表

?> 事件触发顺序: `预检查` -> `Before` -> `处理内容` -> `After`

| 事件                     | Before | After | 描述                 |
| ------------------------ | ------ | ----- | -------------------- |
| PlayerAskCreateLandEvent | √      | √     | 玩家请求创建领地事件 |
| PlayerBuyLandEvent       | √      | √     | 玩家购买领地事件     |
| PlayerEnterLandEvent     | ×      | √     | 玩家进入领地事件     |
| PlayerLeaveLandEvent     | ×      | √     | 玩家离开领地事件     |
| PlayerDeleteLandEvent    | √      | √     | 玩家删除领地事件     |
| LandMemberChangeEvent    | √      | √     | 领地成员变更事件     |
| LandOwnerChangeEvent     | √      | √     | 领地主人变更事件     |
| LandRangeChangeEvent     | √      | √     | 领地范围变更事件     |

!> 非必要请不要修改事件 `const` 修饰的成员，除非你知道你在做什么。

?> 监听事件示例:

```cpp
#include "pland/LandEvent.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/ListenerBase.h"

ll::event::ListenerPtr mEnterLandListener;

void setup() {
    mEnterLandListener = ll::event::EventBus::getInstance().emplaceListener<pland::PlayerEnterLandEvent>([](pland::PlayerEnterLandEvent const& ev) {
        // do something
    });
}
```
