#pragma once
#include "mc/deps/core/common/bedrock/typeid_t.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/registry/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/LandPos.h"
#include "pland/Particle.h"
#include <optional>
#include <unordered_map>


namespace land {

struct LandSelectorData {
    Player* mPlayer{nullptr};
    LandPos mPos;                   // 选择的位置
    int     mDimid;                 // 维度
    bool    mDraw3D;                // 是否绘制3D
    bool    mSelectedPointA{false}; // 是否已经选择了第一个点
    bool    mSelectedPointB{false}; // 是否已经选择了两个点
    bool    mCanDraw{false};        // 能否绘制
    bool    mCanSelect{true};       // 能否选择

    bool     mIsInitedParticle{false}; // 是否已经初始化mParticle
    Particle mParticle;

    LandSelectorData() = default;
    LandSelectorData(Player& player, int dim, bool draw3D) : mPlayer(&player), mDimid(dim), mDraw3D(draw3D) {}
};

class LandSelector {
public:
    LandSelector()                               = default;
    LandSelector(const LandSelector&)            = delete;
    LandSelector& operator=(const LandSelector&) = delete;

    std::unordered_map<UUIDs, LandSelectorData> mSelectors;

    static LandSelector& getInstance();

    bool init();   // 初始化
    bool uninit(); // 卸载

    LandSelectorData* getSelector(Player& player);

    bool isSelectTool(ItemStack const& item) const;
    bool isSelecting(Player& player) const;
    bool isSelected(Player& player) const;
    bool isSelectedPointA(Player& player) const;
    bool isSelectedPointB(Player& player) const;

    bool tryStartSelect(Player& player, int dim, bool draw3D);

    bool trySelectPointA(Player& player, BlockPos pos);

    bool trySelectPointB(Player& player, BlockPos pos);

    bool tryCancel(Player& player);

    bool        completeAndRelease(Player& player);   // 完成选择并释放
    LandDataPtr makeLandFromSelector(Player& player); // 从选择器中生成LandData
};


} // namespace land