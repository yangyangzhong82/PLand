#ifdef LD_DEVTOOL
#pragma once
#include "devtools/components/base/WindowComponent.h"


namespace land {


bool WindowComponent::isOpen() const { return openFlag_; }

bool* WindowComponent::getOpenFlag() { return &openFlag_; }

void WindowComponent::setOpenFlag(bool isOpen) { openFlag_ = isOpen; }

void WindowComponent::tick() {
    if (isOpen()) {
        render();
    }
}

} // namespace land


#endif // LD_DEVTOOL