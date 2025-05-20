#pragma once
#include "IComponent.h"
#include <string>

namespace devtool {

class TextViewer : public IWindow {
    std::string data_;
    int         windowId_;

public:
    explicit TextViewer(std::string data);

    void render() override;
};

} // namespace devtool
