#ifdef LD_DEVTOOL
#pragma once
#include "devtools/components/base/WindowComponent.h"
#include <string>


namespace land {

class TextViewer : public WindowComponent {
    std::string data_;
    int         windowId_ = 0;

public:
    explicit TextViewer(std::string data, int windowId = -1);

    virtual void render() override;
};

} // namespace land


#endif // LD_DEVTOOL