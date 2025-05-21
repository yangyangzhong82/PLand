#pragma once
#include "components/IComponent.h"
#include "pland/Global.h"
#include "pland/LandData.h"

namespace devtool::internals {

class LandCacheViewerWindow;

class LandCacheViewer final : public IMenuElement {
    std::unique_ptr<LandCacheViewerWindow> window_;

public:
    explicit LandCacheViewer();

    bool* getSelectFlag() override;
    bool  isSelected() const override;

    void tick() override;
};


class LandCacheViewerWindow : public IWindow {
    std::unordered_map<land::UUIDs, std::unordered_set<land::LandData_sptr>> lands_;     // 领地缓存
    std::unordered_map<land::UUIDs, std::string>                             realNames_; // 玩家名缓存
    std::unordered_map<land::UUIDs, bool>                                    isShow_;    // 是否显示该玩家的领地

    bool showAllPlayerLand_{true}; // 是否显示所有玩家的领地
    bool showOrdinaryLand_{true};  // 是否显示普通领地
    bool showParentLand_{true};    // 是否显示父领地
    bool showMixLand_{true};       // 是否显示混合领地
    bool showSubLand_{true};       // 是否显示子领地
    int  dimensionFilter_{-1};     // 维度过滤
    int  idFilter_{-1};            // 领地ID过滤

public:
    explicit LandCacheViewerWindow();

    enum Buttons {
        EditLandData,   // 编辑领地数据
        ViewLandData,   // 查看领地数据
        ExportLandData, // 导出领地数据
        LocateChunk     // 定位领地所在区块
    };
    void handleButtonClicked(Buttons bt, land::LandData_sptr land);

    void renderCacheLand(); // 渲染缓存的领地

    void renderToolBar(); // 渲染工具栏

    void preBuildData(); // 预处理数据

    void render() override;

    void tick() override;
};

} // namespace devtool::internals