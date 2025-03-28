#include "nlohmann/json_fwd.hpp"
#include <string>
#ifdef LD_DEVTOOL
#pragma once
#include "devtools/components/CodeEditor.h"
#include "devtools/components/TextViewer.h"
#include "devtools/components/base/Component.h"
#include "devtools/components/base/MenuComponent.h"
#include "devtools/components/base/WindowComponent.h"
#include "fmt/compile.h"
#include "imgui.h"
#include "ll/api/service/PlayerInfo.h"
#include "mod/MyMod.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/PLand.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>


namespace land {

class LandDataEditor : public CodeEditor {
    LandData_sptr land_;

public:
    explicit LandDataEditor(LandData_sptr const& land)
    : CodeEditor(land->toJSON().dump(4), land->getLandID()),
      land_(land) {}

    void _renderMenuBarItem() override {
        CodeEditor::_renderMenuBarItem();

        if (ImGui::BeginMenu("LandData")) {
            ImGui::Text("点击写入按钮应用到领地数据");
            if (ImGui::Button("写入")) {
                auto backup = land_->toJSON();
                try {
                    auto json = nlohmann::json::parse(editor_.GetText());
                    land_->load(json);
                } catch (...) {
                    land_->load(backup);
                    my_mod::MyMod::getInstance().getSelf().getLogger().error("Failed to parse json");
                }
            }
            ImGui::EndMenu();
        }
    }
};


class ShowPlayerLandWindow final : public WindowComponent {
    UUIDs                      uuid_;  // 玩家UUID
    std::string                name_;  // 玩家名称
    std::vector<LandData_sptr> lands_; // 玩家领地

    std::unordered_map<LandID, std::unique_ptr<TextViewer>>     rawDataWindows_; // 查看领地原始数据窗口
    std::unordered_map<LandID, std::unique_ptr<LandDataEditor>> editWindows_;    // 编辑领地窗口

    bool showOrdinaryLand_{true}; // 是否显示普通领地
    bool showParentLand_{true};   // 是否显示父领地
    bool showMixLand_{true};      // 是否显示混合领地
    bool showSubLand_{true};      // 是否显示子领地
    int  dimensionFilter_{-1};    // 维度过滤
    int  idFilter_{-1};           // 领地ID过滤

public:
    explicit ShowPlayerLandWindow(UUIDs uuid) : WindowComponent(), uuid_(std::move(uuid)) {
        auto info = ll::service::PlayerInfo::getInstance().fromUuid(uuid_);
        name_     = info.has_value() ? info->name : uuid_;
    }
    explicit ShowPlayerLandWindow(UUIDs uuid, std::string name)
    : WindowComponent(),
      uuid_(std::move(uuid)),
      name_(std::move(name)) {}

    void updateLands(std::vector<LandData_sptr> lands) {
        this->lands_ = std::move(lands);

        std::unordered_set<LandID> ids;
        for (auto const& ld : lands_) {
            ids.insert(ld->getLandID());
        }

        for (auto iter = rawDataWindows_.begin(); iter != rawDataWindows_.end();) {
            if (ids.find(iter->first) == ids.end()) {
                iter = rawDataWindows_.erase(iter); // 删除不存在的窗口
            } else {
                ++iter;
            }
        }

        for (auto iter = editWindows_.begin(); iter != editWindows_.end();) {
            if (ids.find(iter->first) == ids.end()) {
                iter = editWindows_.erase(iter); // 删除不存在的窗口
            } else {
                ++iter;
            }
        }
    }

    void _handleEdit(LandData_sptr const& land) {
        if (auto iter = editWindows_.find(land->getLandID()); iter == editWindows_.end()) {
            editWindows_.emplace(land->getLandID(), std::make_unique<LandDataEditor>(land));
        }
        auto window = editWindows_[land->getLandID()].get();
        window->setOpenFlag(!window->isOpen());
    }

    void _handleShowRawData(LandData_sptr const& land) {
        if (auto iter = rawDataWindows_.find(land->getLandID()); iter == rawDataWindows_.end()) {
            rawDataWindows_.emplace(
                land->getLandID(),
                std::make_unique<TextViewer>(land->toJSON().dump(4), land->getLandID())
            );
        }
        auto window = rawDataWindows_[land->getLandID()].get();
        window->setOpenFlag(!window->isOpen());
    }

    void _handleExport(LandData_sptr const& land) {
        auto dir = my_mod::MyMod::getInstance().getSelf().getModDir() / "devtools_exports";
        if (!fs::exists(dir)) {
            fs::create_directory(dir);
        }
        auto          file = dir / fmt::format("land_{}.json", land->mLandID);
        std::ofstream ofs(file);
        ofs << land->toJSON().dump(2);
        ofs.close();
    }

    enum ButtonType { Edit, ShowRawData, Export };
    void handleButtonClicked(ButtonType type, LandData_sptr land) {
        switch (type) {
        case Edit:
            _handleEdit(land);
            break;
        case ShowRawData:
            _handleShowRawData(land);
            break;
        case Export:
            _handleExport(land);
            break;
        }
    }

    void _renderLands() {
        {
            ImGui::BeginGroup();
            if (ImGui::Button("重置")) {
                showOrdinaryLand_ = true;
                showParentLand_   = true;
                showMixLand_      = true;
                showSubLand_      = true;
                dimensionFilter_  = -1;
                idFilter_         = -1;
            }
            ImGui::SameLine();
            ImGui::Checkbox("普通领地", &showOrdinaryLand_);
            ImGui::SameLine();
            ImGui::Checkbox("父领地", &showParentLand_);
            ImGui::SameLine();
            ImGui::Checkbox("混合领地", &showMixLand_);
            ImGui::SameLine();
            ImGui::Checkbox("子领地", &showSubLand_);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            ImGui::InputInt("维度过滤", &dimensionFilter_);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            ImGui::InputInt("领地ID过滤", &idFilter_);
            ImGui::EndGroup();
        }

        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable |         // 列大小调整功能
                                       ImGuiTableFlags_Reorderable |       // 表头行中重新排列列
                                       ImGuiTableFlags_Borders |           // 绘制所有边框
                                       ImGuiTableFlags_SizingStretchSame | // 列默认使用 _WidthStretch，默认权重均相等
                                       ImGuiTableFlags_ScrollY;            // 启用垂直滚动，以便固定表头

        if (ImGui::BeginTable("player_lands_table", 6, flags)) {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("类别");
            ImGui::TableSetupColumn("维度");
            ImGui::TableSetupColumn("坐标");
            ImGui::TableSetupColumn("名称");
            ImGui::TableSetupColumn("操作");
            ImGui::TableHeadersRow();

            for (auto const& ld : lands_) {
                if ((!showOrdinaryLand_ && ld->isOrdinaryLand()) || (!showParentLand_ && ld->isParentLand())
                    || (!showMixLand_ && ld->isMixLand()) || (!showSubLand_ && ld->isSubLand())) {
                    continue;
                }
                if ((dimensionFilter_ != -1 && ld->mLandDimid != dimensionFilter_)
                    || (idFilter_ != -1 && ld->mLandID != idFilter_)) {
                    continue;
                }

                // 开始新行
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); // ID
                ImGui::Text("%llu", ld->mLandID);
                ImGui::TableNextColumn(); // 类别
                ImGui::Text(
                    "%s",
                    ld->isOrdinaryLand() ? "普通领地"
                    : ld->isParentLand() ? "父领地"
                    : ld->isMixLand()    ? "混合领地"
                    : ld->isSubLand()    ? "子领地"
                                         : "未知"
                );
                ImGui::TableNextColumn(); // 维度
                ImGui::Text("%i", ld->mLandDimid);
                ImGui::TableNextColumn(); // 坐标
                ImGui::Text("%s", ld->mPos.toString().c_str());
                ImGui::TableNextColumn(); // 名称
                ImGui::Text("%s", ld->getLandName().c_str());
                ImGui::TableNextColumn(); // 操作
                if (ImGui::Button(fmt::format("编辑数据##{}", ld->mLandID).c_str())) {
                    handleButtonClicked(Edit, ld);
                }
                ImGui::SameLine();
                if (ImGui::Button(fmt::format("原始数据##{}", ld->mLandID).c_str())) {
                    handleButtonClicked(ShowRawData, ld);
                }
                ImGui::SameLine();
                if (ImGui::Button(fmt::format("导出##{}", ld->mLandID).c_str())) {
                    handleButtonClicked(Export, ld);
                }
            }
            ImGui::EndTable();
        }
    }

    void render() override {
        if (!ImGui::Begin(fmt::format("玩家: {}", name_).c_str(), this->getOpenFlag(), ImGuiWindowFlags_NoDocking)) {
            ImGui::End();
            return;
        }

        _renderLands();

        ImGui::End();
    }

    void tick() override {
        WindowComponent::tick(); // call base
        for (auto& window : rawDataWindows_) {
            window.second->tick();
        }
        for (auto& window : editWindows_) {
            window.second->tick();
        }
    }
};


class CacheTableWindow final : public WindowComponent {
    std::unordered_set<UUIDs>                                        uuids_;
    std::unordered_map<UUIDs, std::unique_ptr<ShowPlayerLandWindow>> windows_;

public:
    explicit CacheTableWindow() : WindowComponent() {}

    void _filterLands() {
        auto lands = PLand::getInstance().getLands();
        if (lands.size() != uuids_.size()) {
            uuids_.clear();
            uuids_.reserve(lands.size());
            for (auto const& land : lands) {
                uuids_.insert(land->getLandOwner());
            }

            for (auto iter = windows_.begin(); iter != windows_.end();) {
                if (uuids_.find(iter->first) == uuids_.end()) {
                    iter = windows_.erase(iter); // 删除不存在的窗口
                } else {
                    ++iter;
                }
            }
        }
    }

    void handleButtonClicked(UUIDs const& uuid, std::string const& name) {
        auto iter = windows_.find(uuid);
        if (iter == windows_.end()) {
            auto window = std::make_unique<ShowPlayerLandWindow>(uuid, name);
            windows_.emplace(uuid, std::move(window));
        }

        auto window = windows_[uuid].get();
        window->setOpenFlag(!window->isOpen());
    }

    void handleDataUpdate(UUIDs const& uuid, std::vector<LandData_sptr> const& lands) {
        if (auto iter = windows_.find(uuid); iter != windows_.end()) {
            iter->second->updateLands(lands);
        }
    }

    void _renderTable() {
        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable |         // 列大小调整功能
                                       ImGuiTableFlags_Reorderable |       // 表头行中重新排列列
                                       ImGuiTableFlags_Borders |           // 绘制所有边框
                                       ImGuiTableFlags_SizingStretchSame | // 列默认使用 _WidthStretch，默认权重均相等
                                       ImGuiTableFlags_ScrollY;            // 启用垂直滚动，以便固定表头

        if (ImGui::BeginTable("land_cache_table", 3, flags)) {
            ImGui::TableSetupColumn("玩家名称");
            ImGui::TableSetupColumn("领地数量");
            ImGui::TableSetupColumn("操作");
            ImGui::TableHeadersRow(); // 显示表头

            auto& infoDb = ll::service::PlayerInfo::getInstance();
            for (auto const& uuid : uuids_) {
                auto info = infoDb.fromUuid(uuid);
                auto name = info.has_value() ? info->name : uuid;
                auto lds  = PLand::getInstance().getLands(uuid);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%s", std::to_string(lds.size()).c_str());
                ImGui::TableNextColumn();
                if (ImGui::Button(fmt::format("查看领地##{}", uuid).c_str())) {
                    handleButtonClicked(uuid, name);
                }
                handleDataUpdate(uuid, lds);
            }
            ImGui::EndTable();
        }
    }

    void render() override {
        if (!ImGui::Begin("缓存表", this->getOpenFlag())) {
            ImGui::End();
            return;
        }

        _filterLands();

        _renderTable();

        for (auto& [uuid, window] : windows_) {
            window->tick();
        }

        ImGui::End();
    }
};

class DataCacheTableItem final : public TextMenuItemComponent {
    std::unique_ptr<CacheTableWindow> window_;

public:
    explicit DataCacheTableItem() : TextMenuItemComponent("缓存表") { window_ = std::make_unique<CacheTableWindow>(); }

    bool  isSelected() const override { return window_->isOpen(); }
    bool* getSelectFlag() override { return window_->getOpenFlag(); }

    void tick() override { window_->tick(); }
};

class DataMenu final : public MenuComponent {
public:
    explicit DataMenu() : MenuComponent("数据") { this->addItem(std::make_unique<DataCacheTableItem>()); }
};


} // namespace land


#endif