#include "ll/api/schedule/Scheduler.h"
#include "mc/deps/core/common/bedrock/typeid_t.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/registry/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "pland/Global.h"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


namespace land {

class Particle {
public:
    BlockPos    mPos1;
    BlockPos    mPos2;
    int         mDim;
    bool        mDraw3D;
    std::string mParticle;

    bool                  mIsInited{false};
    std::vector<BlockPos> mCalcuatedPos;

    constexpr Particle() = default;
    constexpr Particle(
        BlockPos    pos1,
        BlockPos    pos2,
        int         dim,
        bool        draw3D,
        std::string particle = "minecraft:villager_happy"
    )
    : mPos1(pos1),
      mPos2(pos2),
      mDim(dim),
      mDraw3D(draw3D),
      mParticle(particle) {}

    bool draw(Player& player);
    void fix();
};


struct LandSelectorData {
    Player*  mPlayer{nullptr};
    BlockPos mPos1;
    BlockPos mPos2;
    int      mDim;
    bool     mDraw3D;
    bool     mSelectedPoint1{false};
    bool     mSelectedPoint2{false};
    bool     mCanDraw{false};  // 能否绘制
    bool     mCanSelect{true}; // 能否选择

    Particle mParticle;

    LandSelectorData() = default;
    LandSelectorData(Player& player, int dim, bool draw3D) : mPlayer(&player), mDim(dim), mDraw3D(draw3D) {}
};

class LandSelector {
public:
    LandSelector()                               = default;
    LandSelector(const LandSelector&)            = delete;
    LandSelector& operator=(const LandSelector&) = delete;

    std::string mTools{"minecraft:stick"};

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

    bool trySelectPoint1(Player& player, BlockPos pos);

    bool trySelectPoint2(Player& player, BlockPos pos);

    bool tryStopSelect(Player& player);
};


} // namespace land
