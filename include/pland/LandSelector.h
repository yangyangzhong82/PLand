#include "mc/deps/core/common/bedrock/typeid_t.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/registry/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include "pland/Particle.h"
#include <optional>
#include <unordered_map>


namespace land {

struct LandSelectorData {
    Player*  mPlayer{nullptr};
    BlockPos mPos1;
    BlockPos mPos2;
    int      mDim; // 维度
    bool     mDraw3D;
    bool     mSelectedPoint1{false};
    bool     mSelectedPoint2{false};
    bool     mCanDraw{false};  // 能否绘制
    bool     mCanSelect{true}; // 能否选择

    bool     mIsInited{false};
    Particle mParticle;

    LandSelectorData() = default;
    LandSelectorData(Player& player, int dim, bool draw3D) : mPlayer(&player), mDim(dim), mDraw3D(draw3D) {}
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

    std::optional<LandSelectorData> getSelector(Player& player) const;

    bool isSelectTool(ItemStack const& item) const;
    bool isSelecting(Player& player) const;
    bool isSelected(Player& player) const;
    bool isSelectedPoint1(Player& player) const;
    bool isSelectedPoint2(Player& player) const;

    bool tryStartSelect(Player& player, int dim, bool draw3D);

    bool trySelectPointA(Player& player, BlockPos pos);

    bool trySelectPointB(Player& player, BlockPos pos);

    bool tryCancelSelect(Player& player);
};


} // namespace land