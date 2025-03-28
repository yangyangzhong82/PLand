#ifdef LD_DEVTOOL
#pragma once
#include "devtools/components/base/MenuComponent.h"
#include "imgui.h"
#include <utility>


namespace land {

// constructor
TextMenuItemComponent::TextMenuItemComponent(std::string label) : label_(std::move(label)) {}
TextMenuItemComponent::TextMenuItemComponent(std::string label, std::string shortCut)
: label_(std::move(label)),
  shortCut_(std::move(shortCut)) {}
TextMenuItemComponent::TextMenuItemComponent(std::string label, std::string shortCut, bool selected)
: label_(std::move(label)),
  shortCut_(std::move(shortCut)),
  selected_(selected) {}
TextMenuItemComponent::TextMenuItemComponent(std::string label, std::string shortCut, bool selected, bool enable)
: label_(std::move(label)),
  shortCut_(std::move(shortCut)),
  enable_(enable),
  selected_(selected) {}


std::string const& TextMenuItemComponent::getLabel() const { return label_; }
std::string const& TextMenuItemComponent::getShortCut() const { return shortCut_; }
bool               TextMenuItemComponent::isEnabled() const { return enable_; }
bool               TextMenuItemComponent::isSelected() const { return selected_; } // 是否选中
bool*              TextMenuItemComponent::getSelectFlag() { return &selected_; }
bool*              TextMenuItemComponent::getEnableFlag() { return &enable_; }

void TextMenuItemComponent::render() { ImGui::MenuItem(getLabel().data(), getShortCut().data(), getSelectFlag()); }


//
//   MenuComponent
//
MenuComponent::MenuComponent(std::string label) : label_(std::move(label)) {}

std::string const& MenuComponent::getLabel() const { return label_; }

bool MenuComponent::isEnable() const { return enable_; }

bool* MenuComponent::getEnableFlag() { return &enable_; }

std::vector<std::unique_ptr<IMenuItemComponent>> const& MenuComponent::getItems() const { return items_; }

std::vector<std::unique_ptr<IMenuItemComponent>>& MenuComponent::getItems() { return items_; }

void MenuComponent::addItem(std::unique_ptr<IMenuItemComponent> item) { items_.emplace_back(std::move(item)); }

void MenuComponent::render() {
    for (auto& it : this->items_) {
        if (ImGui::BeginMenu(this->getLabel().data(), this->getEnableFlag())) {
            it->render();
            ImGui::EndMenu();
        }
    }
}
void MenuComponent::tick() {
    for (auto& it : this->items_) {
        it->tick();
    }
}

} // namespace land


#endif