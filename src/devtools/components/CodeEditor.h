#ifdef LD_DEVTOOL
#pragma once
#include "ImGuiColorTextEdit/TextEditor.h"
#include "devtools/components/base/WindowComponent.h"
#include <string>


namespace land {


class CodeEditor : public WindowComponent {
protected:
    TextEditor editor_;
    int        windowId_;

public:
    CodeEditor(std::string const& code, int windowId = -1);

    virtual void _renderMenuBarItem();

    virtual void _renderMenuBar();

    virtual void render() override;
};


} // namespace land


#endif // LD_DEVTOOL