#include <tuple>
#ifdef LD_DEVTOOL
#include "devtools/DevTools.h"
#include "fmt/color.h"
#include "imgui.h"
#include "ll/api/service/PlayerInfo.h"
#include "mc/deps/core/math/Vec3.h"
#include "pland/Global.h"
#include "pland/LandData.h"
#include "pland/PLand.h"
#include "pland/utils/Utils.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace land::devtools::internal {

void RenderLandData(LandData_sptr const& ptr) {
    if (!ImGui::Begin(ptr->getLandName().c_str())) {
        ImGui::End();
        return;
    }
}

void RenderText(string const& data, bool* openFlag) {
    if (!ImGui::Begin("文本查看器", openFlag)) {
        ImGui::End();
        return;
    }
    if (ImGui::Button("复制")) {
        ImGui::SetClipboardText(data.c_str());
    }
    ImGui::SameLine();
    if (ImGui::Button("关闭")) {
        *openFlag = false;
    }
    ImGui::TextUnformatted(data.c_str());
    ImGui::End();
}


void RenderPlayerLands(bool* openFlag, UUIDs const& uuid, std::vector<LandData_sptr> const& lands) {
    auto info = ll::service::PlayerInfo::getInstance().fromUuid(uuid);
    auto name = info.has_value() ? info->name : uuid;

    if (!ImGui::Begin(fmt::format("玩家: {}", name).c_str(), openFlag)) {
        ImGui::End();
        return;
    }

    // 过滤框
    const char* filterOptions[] = {"未选择", "名称", "ID"};
    static int  filterType      = 0;

    static string nameFilter;
    static int    idFilter;
    nameFilter.resize(128);
    {
        ImGui::BeginGroup();
        ImGui::Combo("过滤方式", &filterType, filterOptions, IM_ARRAYSIZE(filterOptions));
        if (filterType == 1) {
            ImGui::InputText("名称", nameFilter.data(), nameFilter.size());
        } else {
            ImGui::InputInt("ID", &idFilter);
        }
        ImGui::EndGroup();
    }


    // 渲染领地列表
    static ImGuiTableFlags flags = ImGuiTableFlags_Resizable |         // 列大小调整功能
                                   ImGuiTableFlags_Reorderable |       // 表头行中重新排列列
                                   ImGuiTableFlags_Borders |           // 绘制所有边框
                                   ImGuiTableFlags_SizingStretchSame | // 列默认使用 _WidthStretch，默认权重均相等
                                   ImGuiTableFlags_ScrollY;            // 启用垂直滚动，以便固定表头

    ImGui::BeginTable("player_lands_table", 5, flags);
    ImGui::TableSetupColumn("ID");
    ImGui::TableSetupColumn("维度");
    ImGui::TableSetupColumn("坐标");
    ImGui::TableSetupColumn("名称");
    ImGui::TableSetupColumn("操作");

    ImGui::TableHeadersRow();

    static std::unordered_map<LandID, bool[4]> isOpen; // [详细信息] [原始数据] [删除] [导出]
    for (auto const& ld : lands) {
        if ((filterType == 1 && ld->getLandName().find(nameFilter) == string::npos)
            || (filterType == 2 && ld->mLandID != static_cast<LandID>(idFilter))) {
            continue;
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%llu", ld->mLandID);
        ImGui::TableNextColumn();
        ImGui::Text("%i", ld->mLandDimid);
        ImGui::TableNextColumn();
        ImGui::Text("%s", ld->mPos.toString().c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s", ld->getLandName().c_str());
        ImGui::TableNextColumn();
        if (ImGui::Button(fmt::format("详细信息 ##{}", ld->mLandID).c_str())) {}
        ImGui::SameLine();
        if (ImGui::Button(fmt::format("原始数据 ##{}", ld->mLandID).c_str())) {
            isOpen[ld->mLandID][1] = !isOpen[ld->mLandID][1];
        }
        if (isOpen[ld->mLandID][1]) RenderText(ld->toJSON().dump(2), &isOpen[ld->mLandID][1]);
        ImGui::SameLine();
        if (ImGui::Button(fmt::format("删除 ##{}", ld->mLandID).c_str())) {}
        ImGui::SameLine();
        if (ImGui::Button(fmt::format("导出 ##{}", ld->mLandID).c_str())) {}
    }
    ImGui::EndTable();

    ImGui::End();
}


void ShowLandCacheWindow(bool* open) {
    if (!ImGui::Begin("领地缓存表", open)) {
        ImGui::End();
        return;
    }

    static std::unordered_set<UUIDs> uuids;
    {
        auto lds = PLand::getInstance().getLands();
        if (lds.size() != uuids.size()) {
            uuids.clear();
            uuids.reserve(lds.size());
            for (auto const& ld : lds) {
                if (uuids.find(ld->getLandOwner()) == uuids.end()) {
                    uuids.insert(ld->getLandOwner());
                }
            }
        }
    }

    // 渲染玩家列表
    static std::unordered_map<UUIDs, bool> isOpens;
    {
        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable |         // 列大小调整功能
                                       ImGuiTableFlags_Reorderable |       // 表头行中重新排列列
                                       ImGuiTableFlags_Borders |           // 绘制所有边框
                                       ImGuiTableFlags_SizingStretchSame | // 列默认使用 _WidthStretch，默认权重均相等
                                       ImGuiTableFlags_ScrollY;            // 启用垂直滚动，以便固定表头

        ImGui::BeginTable("land_cache_table", 3, flags);
        ImGui::TableSetupColumn("玩家名称");
        ImGui::TableSetupColumn("领地数量");
        ImGui::TableSetupColumn("操作");

        ImGui::TableHeadersRow(); // 显示表头

        auto& infos = ll::service::PlayerInfo::getInstance();
        for (auto const& uuid : uuids) {
            auto info = infos.fromUuid(uuid);
            auto name = info.has_value() ? info->name : uuid;
            auto lds  = PLand::getInstance().getLands(uuid);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", name.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", fmt::format("{}", lds.size()).c_str());
            ImGui::TableNextColumn();
            if (ImGui::Button(fmt::format("查看领地 ##{}", uuid).c_str())) {
                isOpens[uuid] = !isOpens[uuid];
            }
            if (isOpens[uuid]) RenderPlayerLands(&isOpens[uuid], uuid, lds);
        }
        ImGui::EndTable();
    }

    ImGui::End();
}


} // namespace land::devtools::internal
#endif // LD_DEVTOOL