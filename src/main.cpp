#include "src/ui/window.h"
#include <cstdlib>

namespace {
class WhatIsPlugin final : public winshun::Plugin {
public:
    WhatIsPlugin() : m_window(
        [this](const std::string& name,const winshun::Json& args,std::function<void(winshun::Json)> reply){return toolAsync(name,args,std::move(reply));},
        [this](const std::string& id){cancelTool(id);}) {}
    ~WhatIsPlugin() override { m_window.stop(); }
protected:
    std::string name() const override { return "ra9fael.whatis"; }
    void initialized() override {
        m_ready=m_window.start();
        appearanceChanged();
    }
    void appearanceChanged() override { m_window.appearance(whatis::Look::from(appearance(),theme(),!chinese())); }
    void shuttingDown() override { m_window.stop(); }
    winshun::Items query(const winshun::Query& q) override {
        if(q.command!="whatis")return {};
        winshun::Item item("whatis.pick",chinese()?"选取窗口，看看这是什么":"Pick a window to identify its program",
            chinese()?"移动预览，单击锁定 · Esc 取消；结果可复制路径、定位文件":"Hover to preview, click to lock, Esc to cancel; copy or reveal the EXE");
        item.withIcon("glyph:U+E71E").withScore(100).withActions({{"pick",chinese()?"选取窗口":"Pick a window","Enter"}});
        if(!m_ready)item.subtitle=chinese()?"原生界面未能启动，请停用后重试":"The native UI could not start; disable and retry";
        return {item};
    }
    winshun::Effects action(const winshun::ActionRequest& request) override {
        if(request.command!="whatis"||request.action!="pick")return {winshun::effect::notify(chinese()?"不支持的操作":"Unsupported action")};
        if(!m_ready||!m_window.pick())return {winshun::effect::notify(chinese()?"无法启动选取界面":"Could not start the picker")};
        return {winshun::effect::hide()};
    }
private:
    whatis::Window m_window;
    bool m_ready=false;
};
}
int main(int argc,char** argv) {
    whatis::enableDpiAwareness();
    // Read-only diagnostic entry point, also used by the native contract tests.
    // It does not select a window or perform clipboard / shell actions.
    if(argc==3&&std::string(argv[1])=="--probe") {
        char* end=nullptr;
        const auto number=std::strtoull(argv[2],&end,0);
        if(!number||!end||*end)return 2;
        const auto id=whatis::identify(reinterpret_cast<HWND>(number));
        auto result=whatis::inspect(id);
        winshun::detail::writeLine(whatis::resultJson(result).dump());
        return result.hasPath()?0:1;
    }
    return WhatIsPlugin().serve();
}
