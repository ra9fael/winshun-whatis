#include "window.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <objidl.h>
#include <gdiplus.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace whatis {
namespace {
constexpr UINT Dispatch = WM_APP + 10, Stop = WM_APP + 11;
constexpr int Pick = 100, Copy = 101, Reveal = 102, Details = 103, Diagnostics = 104, Close = 105;
constexpr int Exe = 200, Path = 201, Title = 202, Meta = 203, Technical = 204, Note = 205, Status = 206, Heading = 207, PathLabel = 208;
constexpr wchar_t ClassName[] = L"WinShun.WhatIs.Window";
COLORREF blend(COLORREF a, COLORREF b, int weight) {
    return RGB((GetRValue(a)*(100-weight)+GetRValue(b)*weight)/100,
        (GetGValue(a)*(100-weight)+GetGValue(b)*weight)/100,
        (GetBValue(a)*(100-weight)+GetBValue(b)*weight)/100);
}
Gdiplus::Color ink(COLORREF color) { return Gdiplus::Color(255,GetRValue(color),GetGValue(color),GetBValue(color)); }
void roundBox(Gdiplus::Graphics& g, Gdiplus::RectF r, float radius, COLORREF fill, COLORREF border, bool outline=true) {
    Gdiplus::GraphicsPath path;
    const float d=std::min({radius*2,r.Width,r.Height});
    if(d>0) {
        path.AddArc(r.X,r.Y,d,d,180,90);path.AddArc(r.GetRight()-d,r.Y,d,d,270,90);
        path.AddArc(r.GetRight()-d,r.GetBottom()-d,d,d,0,90);path.AddArc(r.X,r.GetBottom()-d,d,d,90,90);path.CloseFigure();
    } else path.AddRectangle(r);
    Gdiplus::SolidBrush brush(ink(fill));g.FillPath(&brush,&path);
    if(outline){Gdiplus::Pen pen(ink(border),1);g.DrawPath(&pen,&path);}
}
// Code-drawn icons stay sharp at every scale and require no file reads.
void glyph(Gdiplus::Graphics& g,int kind,float x,float y,float size,COLORREF color,bool expanded=false) {
    const auto saved=g.Save();g.TranslateTransform(x,y);g.ScaleTransform(size/24,size/24);
    Gdiplus::Pen pen(ink(color),1.65f);pen.SetStartCap(Gdiplus::LineCapRound);pen.SetEndCap(Gdiplus::LineCapRound);pen.SetLineJoin(Gdiplus::LineJoinRound);
    if(kind==Pick) {
        g.DrawEllipse(&pen,5.f,5.f,14.f,14.f);
        g.DrawLine(&pen,12.f,2.f,12.f,7.f);g.DrawLine(&pen,12.f,17.f,12.f,22.f);
        g.DrawLine(&pen,2.f,12.f,7.f,12.f);g.DrawLine(&pen,17.f,12.f,22.f,12.f);
        Gdiplus::SolidBrush dot(ink(color));g.FillEllipse(&dot,10.5f,10.5f,3.f,3.f);
    } else if(kind==Copy||kind==Diagnostics) {
        g.DrawRectangle(&pen,8.f,8.f,12.f,13.f);g.DrawLine(&pen,4.f,16.f,4.f,3.f);g.DrawLine(&pen,4.f,3.f,15.f,3.f);
    } else if(kind==Reveal) {
        Gdiplus::PointF p[]={{3,7},{3,19},{21,19},{21,7},{12,7},{10,4},{3,4},{3,7}};g.DrawLines(&pen,p,8);
    } else if(kind==Details) {
        if(expanded){g.DrawLine(&pen,6.f,14.f,12.f,8.f);g.DrawLine(&pen,12.f,8.f,18.f,14.f);}
        else {g.DrawLine(&pen,6.f,9.f,12.f,15.f);g.DrawLine(&pen,12.f,15.f,18.f,9.f);}
    } else if(kind==Close) {
        g.DrawLine(&pen,7.f,7.f,17.f,17.f);g.DrawLine(&pen,17.f,7.f,7.f,17.f);
    } else {
        g.DrawRectangle(&pen,3.f,4.f,18.f,16.f);g.DrawLine(&pen,3.f,9.f,21.f,9.f);
        Gdiplus::SolidBrush dot(ink(color));g.FillEllipse(&dot,5.5f,6.f,1.5f,1.5f);g.FillEllipse(&dot,8.5f,6.f,1.5f,1.5f);
        g.DrawLine(&pen,8.f,13.f,6.f,15.f);g.DrawLine(&pen,6.f,15.f,8.f,17.f);g.DrawLine(&pen,11.f,17.f,15.f,17.f);
    }
    g.Restore(saved);
}
std::wstring hex(std::uint64_t value) { std::wostringstream out; out << L"0x" << std::hex << std::uppercase << value; return out.str(); }
COLORREF color(const winshun::Json& look, const char* name, COLORREF fallback, COLORREF base) {
    const auto text = look[name].toString();
    if (text.size() != 7 && text.size() != 9) return fallback;
    if (text[0] != '#') return fallback;
    char* end = nullptr;
    const auto value = std::strtoul(text.c_str() + 1, &end, 16);
    if (end != text.c_str() + text.size()) return fallback;
    const unsigned alpha = text.size() == 9 ? (value >> 24) & 255 : 255;
    const auto channel = [alpha](unsigned fg, unsigned bg) { return (fg * alpha + bg * (255 - alpha) + 127) / 255; };
    return RGB(channel((value >> 16) & 255, GetRValue(base)), channel((value >> 8) & 255, GetGValue(base)), channel(value & 255, GetBValue(base)));
}
std::wstring systemError(DWORD error, bool english) {
    if (error == ERROR_ACCESS_DENIED) return english ? L"Access denied (5)" : L"访问被拒绝（5）";
    if (error == ERROR_INVALID_WINDOW_HANDLE) return english ? L"Window closed or changed" : L"窗口已关闭或发生变化";
    return (english ? L"Windows error " : L"Windows 错误 ") + std::to_wstring(error);
}
}
Look Look::from(const winshun::Json& appearance, const std::string& theme, bool english) {
    Look out; out.english = english; out.dark = theme == "dark";
    if (out.dark) {
        out.background = RGB(24,28,36); out.panel = RGB(34,39,50); out.text = RGB(238,241,247);
        out.secondary = RGB(169,179,197); out.border = RGB(58,66,80); out.accent = RGB(154,195,255);
        out.accentFill = RGB(177,208,255); out.accentText = RGB(19,41,71);
    }
    const auto& look = appearance["nativeLook"];
    out.rounded = appearance["corners"].toString() != "square" && appearance["corners"].toString() != "sharp";
    if (look["rounded"].isBool()) out.rounded = look["rounded"].toBool();
    out.radius = out.rounded ? static_cast<int>(std::clamp(look["controlRadius"].toInt(6), 0LL, 16LL)) : 0;
    out.background = color(look, "background", out.background, out.background);
    out.panel = color(look, "layer", out.panel, out.background);
    out.text = color(look, "text", out.text, out.background);
    out.secondary = color(look, "subtext", out.secondary, out.background);
    out.border = color(look, "divider", out.border, out.background);
    out.accent = color(look, "accent", out.accent, out.background);
    out.accentFill = color(look, "accentFill", out.accentFill, out.background);
    out.accentText = color(look, "accentText", out.accentText, out.accentFill);
    const auto size = appearance["uiSize"].toString();
    if (size == "roomy" || size == "large") out.scale = 1.15f;
    else if (size == "compact" || size == "small") out.scale = .92f;
    return out;
}
winshun::Json resultJson(const Result& result) {
    return winshun::Json::Object{{"hwnd", utf8(hex(result.identity.hwnd))}, {"pid", static_cast<long long>(result.identity.pid)},
        {"rootHwnd", utf8(hex(result.root))}, {"rootPid", static_cast<long long>(result.rootPid)},
        {"created", std::to_string(result.created)}, {"title", utf8(result.title)}, {"class", utf8(result.windowClass)},
        {"path", utf8(result.path)}, {"exe", utf8(result.executable())}, {"error", static_cast<long long>(result.error)},
        {"stale", result.stale}, {"host", result.knownHost()}};
}
std::wstring diagnosticText(const Result& result, bool english) {
    return std::wstring(english ? L"What Is This — window snapshot\r\n" : L"这是什么 — 窗口快照\r\n") +
        (english ? L"Title: " : L"标题：") + result.title + L"\r\nPID: " + std::to_wstring(result.identity.pid) +
        L"\r\nHWND: " + hex(result.identity.hwnd) + L"\r\nClass: " + result.windowClass +
        L"\r\nRoot HWND: " + hex(result.root) + L"\r\nRoot PID: " + std::to_wstring(result.rootPid) +
        L"\r\nEXE: " + result.path + L"\r\nError: " + std::to_wstring(result.error) +
        L"\r\nCreation: " + std::to_wstring(result.created) + L"\r\nStale: " + (result.stale ? L"true" : L"false") +
        L"\r\nHost: " + (result.knownHost() ? L"true" : L"false");
}

Window::Window(Tool tool, CancelTool cancel) : m_tool(std::move(tool)), m_cancelTool(std::move(cancel)) {}
Window::~Window() { stop(); }
bool Window::start() {
    std::unique_lock lock(m_mutex);
    if (m_thread.joinable()) return m_controller != nullptr;
    m_thread = std::thread([this] { run(); });
    m_readyCondition.wait(lock, [this] { return m_ready; });
    if (!m_controller) return false;
    m_worker = std::thread([this] { probeLoop(); });
    return true;
}
void Window::stop() {
    HWND controller;
    {
        const std::lock_guard lock(m_mutex);
        m_stopping = true; m_workerStop = true; m_work.reset(); m_queue.clear(); controller = m_controller;
    }
    m_probeCondition.notify_all();
    if (controller) PostMessageW(controller, Stop, 0, 0);
    if (m_worker.joinable()) m_worker.join();
    if (m_thread.joinable()) m_thread.join();
}
bool Window::post(std::function<void()> task) {
    const std::lock_guard lock(m_mutex);
    if (m_stopping || !m_controller) return false;
    m_queue.push_back(std::move(task));
    if (!PostMessageW(m_controller, Dispatch, 0, 0)) { m_queue.pop_back(); return false; }
    return true;
}
void Window::drain() {
    std::deque<std::function<void()>> tasks;
    { const std::lock_guard lock(m_mutex); tasks.swap(m_queue); }
    for (auto& task : tasks) task();
}
bool Window::pick() { return post([this] { SetTimer(m_controller,3,120,nullptr); }); }
void Window::appearance(Look look) { post([this, look] {
    if (look.english != m_look.english) m_status.clear();
    m_look = look; applyLook(); updateCard();
}); }
std::optional<Result> Window::snapshot() const { const std::lock_guard lock(m_mutex); return m_snapshot; }
void Window::probeLoop() {
    for (;;) {
        Ticket ticket;
        {
            std::unique_lock lock(m_mutex);
            m_probeCondition.wait(lock, [this] { return m_workerStop || m_work.has_value(); });
            if (m_workerStop) return;
            ticket = *m_work; m_work.reset();
        }
        auto result = inspect(ticket.identity);
        post([this, ticket, result = std::move(result)]() mutable {
            if (!m_selection.accept(ticket, std::move(result))) return;
            if (m_selection.phase == Phase::Locked) {
                { const std::lock_guard lock(m_mutex); m_snapshot = m_selection.locked; }
                m_status = tr(L"已锁定窗口", L"Window locked");
                showCard();
            }
            if (m_hint) InvalidateRect(m_hint, nullptr, TRUE);
        });
    }
}
void Window::requestProbe() {
    if (!m_selection.candidate) return;
    { const std::lock_guard lock(m_mutex); m_work = m_selection.ticket(); }
    m_probeCondition.notify_one(); m_probeQueued = true;
}
int Window::px(int value) const { return static_cast<int>(std::lround(value * m_dpi / 96.0 * m_look.scale)); }
void Window::clearGraphics() {
    for(HFONT font:{m_font,m_large,m_small,m_heading,m_mono})if(font)DeleteObject(font);
    if (m_background) DeleteObject(m_background);
    if (m_panel) DeleteObject(m_panel);
    if (m_soft) DeleteObject(m_soft);
    m_font = m_large = m_small = m_heading = m_mono = nullptr; m_background = m_panel = m_soft = nullptr;
}
void Window::applyLook() {
    if (!m_card) return;
    clearGraphics();
    const auto font=[this](int size,int weight,const wchar_t* family){return CreateFontW(-px(size),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,family);};
    const wchar_t* family=m_look.english?L"Segoe UI":L"Microsoft YaHei UI";
    m_font=font(14,FW_NORMAL,family);m_large=font(21,FW_SEMIBOLD,family);
    m_small=font(12,FW_NORMAL,family);m_heading=font(14,FW_SEMIBOLD,family);
    m_mono=font(12,FW_NORMAL,L"Consolas");
    m_softColor=blend(m_look.background,m_look.text,m_look.dark?5:3);
    m_background = CreateSolidBrush(m_look.background); m_panel = CreateSolidBrush(m_look.panel);
    m_soft=CreateSolidBrush(m_softColor);
    for (int id : {Pick, Copy, Reveal, Details, Diagnostics, Close, Exe, Path, Title, Meta, Technical, Note, Status, Heading, PathLabel}) {
        HFONT chosen=id==Exe?m_large:id==Heading?m_heading:id==Technical?m_mono:
            (id==Meta||id==Note||id==Status||id==PathLabel)?m_small:m_font;
        SendMessageW(control(id),WM_SETFONT,reinterpret_cast<WPARAM>(chosen),TRUE);
    }
    BOOL dark = m_look.dark;
    DwmSetWindowAttribute(m_card, 20, &dark, sizeof dark);
    const int corners = m_look.rounded ? 2 : 1;
    DwmSetWindowAttribute(m_card, 33, &corners, sizeof corners);
    const MARGINS margins{1,1,1,1};DwmExtendFrameIntoClientArea(m_card,&margins);
    SetWindowPos(m_card,nullptr,0,0,0,0,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SetWindowTextW(m_card, tr(L"这是什么 · Win顺", L"What Is This · Winshun").c_str());
    layout(); InvalidateRect(m_card, nullptr, TRUE);
}
void Window::move(int id, int x, int y, int width, int height, bool visible) {
    const HWND child = control(id);
    MoveWindow(child, px(x), px(y), px(width), px(height), TRUE);
    ShowWindow(child, visible ? SW_SHOW : SW_HIDE);
}
bool Window::hasNotice() const {
    const auto& data=m_selection.locked;
    return data&&(data->error||data->knownHost()||data->stale||(data->rootPid&&data->rootPid!=data->identity.pid));
}
bool Window::hasStatus() const {
    return !m_status.empty()&&m_status!=L"已锁定窗口"&&m_status!=L"Window locked"&&m_status!=L"正在读取进程…"&&m_status!=L"Reading process…";
}
int Window::footerY() const { return hasNotice()?380:332; }
int Window::cardHeight() const {
    if(!m_selection.locked||m_selection.phase==Phase::Resolving)return 310+(hasStatus()?44:0);
    return footerY()+(m_details?226:56)+(hasStatus()?52:0);
}
void Window::layout() {
    const bool ready=m_selection.locked&&m_selection.phase!=Phase::Resolving;
    const int height=cardHeight(),footer=footerY();
    SetWindowPos(m_card,nullptr,0,0,px(480),px(height),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    move(Heading,52,17,330,22);move(Close,432,10,34,34);
    SetWindowLongPtrW(control(Exe),GWL_STYLE,(GetWindowLongPtrW(control(Exe),GWL_STYLE)&~SS_TYPEMASK)|(ready?SS_LEFT:SS_CENTER)|SS_ENDELLIPSIS);
    SetWindowLongPtrW(control(Note),GWL_STYLE,(GetWindowLongPtrW(control(Note),GWL_STYLE)&~SS_TYPEMASK)|(ready?SS_LEFT:SS_CENTER));
    move(Exe,ready?84:24,ready?74:150,ready?372:432,30);
    move(Pick,ready?302:132,ready?footer+10:246,ready?154:216,ready?34:40);
    move(Meta,84,109,372,21,ready);move(Title,24,142,432,22,ready);
    move(PathLabel,40,187,400,18,ready);move(Path,40,212,400,44,ready);
    move(Copy,24,278,210,38,ready);move(Reveal,246,278,210,38,ready);
    move(Details,24,footer+10,164,34,ready);
    move(Technical,40,footer+66,400,78,ready&&m_details);
    const bool notice=hasNotice();
    move(Note,24,ready?(notice?330:footer+156):191,432,38,!ready||notice||m_details);
    move(Diagnostics,24,footer+190,240,30,ready&&m_details);
    move(Status,24,height-(ready?44:36),432,ready?38:28,hasStatus());
    // Keep the resized result on the monitor's work area.
    MONITORINFO info{}; info.cbSize=sizeof info; RECT rect{};
    GetMonitorInfoW(MonitorFromWindow(m_card, MONITOR_DEFAULTTONEAREST), &info); GetWindowRect(m_card, &rect);
    const int x = std::clamp<int>(rect.left, info.rcWork.left, std::max<int>(info.rcWork.left, info.rcWork.right - (rect.right - rect.left)));
    const int y = std::clamp<int>(rect.top, info.rcWork.top, std::max<int>(info.rcWork.top, info.rcWork.bottom - (rect.bottom - rect.top)));
    SetWindowPos(m_card,nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    for(int id:{Path,Technical}) {RECT client{};GetClientRect(control(id),&client);SendMessageW(control(id),EM_SETRECT,0,reinterpret_cast<LPARAM>(&client));}
    InvalidateRect(m_card,nullptr,FALSE);
}
void Window::status(std::wstring text) { const bool before=hasStatus();m_status = std::move(text); set(Status,m_status);if(before!=hasStatus())layout(); }
void Window::updateCard() {
    if (!m_card) return;
    SetWindowTextW(m_card,tr(L"这是什么 · Win顺",L"What Is This · Winshun").c_str());
    set(Heading,tr(L"这是什么",L"What Is This"));
    set(Close,tr(L"关闭",L"Close"));
    set(Pick,m_selection.locked ? tr(L"重新选取",L"Pick again") : tr(L"选取窗口",L"Pick a window"));
    set(Copy,tr(L"复制路径",L"Copy path")); set(Reveal,tr(L"定位文件",L"Reveal file"));
    set(Details,m_details ? tr(L"收起详情",L"Hide details") : tr(L"窗口详情",L"Window details"));
    set(Diagnostics,tr(L"复制诊断（含路径）",L"Copy details (with path)"));
    set(PathLabel,tr(L"可执行文件路径",L"Executable path")); set(Status,m_status);
    const auto& data = m_selection.locked;
    const bool resolving = m_selection.phase == Phase::Resolving;
    EnableWindow(control(Copy),data && data->hasPath() && !resolving);
    EnableWindow(control(Reveal),data && data->hasPath() && !resolving && m_pendingTool.empty());
    EnableWindow(control(Diagnostics),data.has_value() && !resolving);
    if (resolving || !data) {
        set(Exe,resolving ? tr(L"正在读取…",L"Reading…") : tr(L"指一指，就知道",L"Point at a window"));
        for(int id : {Path,Title,Meta,Technical,Note}) set(id,L"");
        set(Note,resolving?tr(L"正在确认窗口背后的程序",L"Identifying the program behind this window"):
            tr(L"选取一个窗口，查看它来自哪个程序",L"Pick a window to see which program it belongs to"));
        layout();
        return;
    }
    const auto& result = *data;
    set(Exe,result.hasPath() ? result.executable() : tr(L"路径暂不可读",L"Path unavailable"));
    set(Path,result.hasPath() ? result.path : systemError(result.error,m_look.english));
    set(Title,(tr(L"窗口 · ",L"Window · ")) + (result.title.empty() ? tr(L"无标题",L"Untitled") : result.title));
    set(Meta,L"PID " + std::to_wstring(result.identity.pid) + L"  ·  " + (result.stale ? tr(L"窗口已关闭 · 历史快照",L"Window closed · snapshot") : result.error?tr(L"访问受限",L"Restricted access"):tr(L"已识别",L"Identified")));
    std::wstring note;
    if(result.stale)note=tr(L"窗口已关闭，当前保留上次识别结果。",L"This window closed. The last result is kept as a snapshot.");
    else if (result.knownHost()) note = tr(L"这是窗口宿主。内容应用尚未确认。",L"This is a window host. The content app is unresolved.");
    else if (result.error) note = tr(L"保留已确认的窗口与 PID；无法读取路径。",L"Window and PID are known; the path could not be read.");
    else if (result.rootPid && result.rootPid != result.identity.pid) note = tr(L"当前为跨进程子窗口；顶层窗口属于另一个 PID。",L"Cross-process child; the root window has another PID.");
    else note = tr(L"显示创建此窗口的进程，不推断网页或终端内的程序。",L"The window's process, not programs within a page or terminal.");
    set(Note,note);
    set(Technical,L"HWND  " + hex(result.identity.hwnd) + L"\r\nClass  " + result.windowClass + L"\r\nRoot  " + hex(result.root) + L"   PID " + std::to_wstring(result.rootPid) +
        L"\r\n" + (result.error ? systemError(result.error,m_look.english) : tr(L"进程映像路径已读取",L"Process image path retrieved")));
    layout();
}
void Window::showCard() {
    updateCard(); ShowWindow(m_card,SW_SHOWNORMAL);
    SetWindowPos(m_card,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    SetForegroundWindow(m_card); SetFocus(control(Pick));
    SetTimer(m_controller,2,1000,nullptr);
}

void Window::begin(bool drag) {
    if (m_picking) return;
    if (!inputDesktopAvailable()) { status(tr(L"当前输入桌面不可访问",L"Input desktop unavailable")); showCard(); return; }
    if (!m_pendingTool.empty()) { m_cancelTool(m_pendingTool); m_pendingTool.clear(); ++m_toolGeneration; }
    m_selection.begin(); m_probeQueued = false; m_drag = drag; m_armed = drag; m_down = {};
    ShowWindow(m_card,SW_HIDE); KillTimer(m_controller,2);
    m_started = GetTickCount64(); m_candidateSince = m_started;
    m_picking = true;
    std::vector<RECT> monitors;
    EnumDisplayMonitors(nullptr,nullptr,[](HMONITOR,HDC,LPRECT rect,LPARAM data)->BOOL {
        reinterpret_cast<std::vector<RECT>*>(data)->push_back(*rect); return TRUE;
    },reinterpret_cast<LPARAM>(&monitors));
    for (const RECT& rect : monitors) {
        HWND shield = CreateWindowExW(WS_EX_LAYERED|WS_EX_TOPMOST|WS_EX_TOOLWINDOW,ClassName,L"WhatIsInput",WS_POPUP,
            rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,GetModuleHandleW(nullptr),this);
        if (!shield || !SetLayeredWindowAttributes(shield,0,1,LWA_ALPHA)) {
            if (shield) DestroyWindow(shield);
            cancel(tr(L"无法建立选取输入层",L"Could not create the picker")); return;
        }
        m_shields.push_back(shield); ShowWindow(shield,SW_SHOWNOACTIVATE);
    }
    if (m_shields.empty()) { cancel(tr(L"没有可用的显示器",L"No display available")); return; }
    POINT point{}; GetCursorPos(&point);
    HWND active = m_shields.front();
    for(HWND shield:m_shields){RECT rect{};GetWindowRect(shield,&rect);if(PtInRect(&rect,point))active=shield;}
    SetForegroundWindow(active); SetFocus(active);
    if (drag) SetCapture(active);
    for(auto& edge:m_edges) {
        edge = CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,ClassName,L"WhatIsBorder",WS_POPUP,0,0,0,0,nullptr,nullptr,GetModuleHandleW(nullptr),this);
        if(edge)SetLayeredWindowAttributes(edge,0,255,LWA_ALPHA);
    }
    m_hint = CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,ClassName,L"WhatIsHint",WS_POPUP,0,0,px(340),px(80),nullptr,nullptr,GetModuleHandleW(nullptr),this);
    if(m_hint)SetLayeredWindowAttributes(m_hint,0,255,LWA_ALPHA);
    SetTimer(m_controller,1,33,nullptr); tick();
}
void Window::endInput() {
    m_picking = false; KillTimer(m_controller,1); ReleaseCapture();
    for(HWND shield:m_shields)DestroyWindow(shield);
    m_shields.clear();
    for(auto& edge:m_edges){if(edge)DestroyWindow(edge);edge=nullptr;}
    if(m_hint)DestroyWindow(m_hint);
    m_hint=nullptr;
    m_armed = false; m_drag = false;
}
void Window::cancel(const std::wstring& reason) {
    m_selection.cancel(); endInput(); status(reason.empty()?tr(L"已取消选取",L"Selection canceled"):reason); showCard();
}
void Window::confirm() {
    if (!m_selection.confirm()) return;
    endInput(); status(tr(L"正在读取进程…",L"Reading process…")); showCard(); requestProbe();
}
void Window::tick() {
    if (!m_picking) return;
    const auto now = GetTickCount64();
    if (now - m_started > 30000) { cancel(tr(L"选取已超时（30 秒）",L"Selection timed out (30 s)")); return; }
    if (!inputDesktopAvailable()) { cancel(tr(L"输入桌面已切换",L"Input desktop changed")); return; }
    if (now - m_started > 200 && identify(GetForegroundWindow()).pid != GetCurrentProcessId()) {
        cancel(tr(L"请点击“选取窗口”继续",L"Click Pick a window to continue")); return;
    }
    POINT point{}; if(!GetCursorPos(&point)) {cancel();return;}
    if (!m_armed || m_drag) {
        const HWND hit = hitWindow(point,GetCurrentProcessId());
        if(m_selection.point(identify(hit))){m_candidateSince=now;m_probeQueued=false;}
    }
    if(m_selection.candidate && !m_probeQueued && now-m_candidateSince>=90)requestProbe();
    updateHighlight(point);
}
void Window::updateHighlight(POINT point) {
    const auto id = m_selection.candidate;
    if(id) {
        RECT rect = frameRect(reinterpret_cast<HWND>(id.hwnd)); const int thick=std::max(2,px(2));
        RECT bars[4]={{rect.left,rect.top,rect.right,rect.top+thick},{rect.left,rect.bottom-thick,rect.right,rect.bottom},
            {rect.left,rect.top,rect.left+thick,rect.bottom},{rect.right-thick,rect.top,rect.right,rect.bottom}};
        for(int i=0;i<4;++i)if(m_edges[i])SetWindowPos(m_edges[i],HWND_TOPMOST,bars[i].left,bars[i].top,bars[i].right-bars[i].left,bars[i].bottom-bars[i].top,SWP_NOACTIVATE|SWP_SHOWWINDOW);
    } else for(HWND edge:m_edges)if(edge)ShowWindow(edge,SW_HIDE);
    MONITORINFO info{};info.cbSize=sizeof info;GetMonitorInfoW(MonitorFromPoint(point,MONITOR_DEFAULTTONEAREST),&info);
    const int width=px(340),height=px(80);
    int x=point.x+px(20),y=point.y+px(26);
    if(x+width>info.rcMonitor.right)x=point.x-width-px(20);
    if(y+height>info.rcMonitor.bottom)y=point.y-height-px(20);
    x=std::max<int>(info.rcMonitor.left,x);y=std::max<int>(info.rcMonitor.top,y);
    if(m_hint){SetWindowPos(m_hint,HWND_TOPMOST,x,y,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);InvalidateRect(m_hint,nullptr,FALSE);}
}

void Window::copy(bool diagnostics) {
    if(!m_selection.locked)return;
    const auto& result=*m_selection.locked;
    if(!diagnostics&&!result.hasPath())return;
    const std::wstring text=diagnostics?diagnosticText(result,m_look.english):result.path;
    const SIZE_T bytes=(text.size()+1)*sizeof(wchar_t);
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);
    if(!memory){status(tr(L"复制失败：内存不足",L"Copy failed: out of memory"));return;}
    void* target=GlobalLock(memory);
    if(!target){GlobalFree(memory);status(tr(L"无法准备复制内容",L"Could not prepare clipboard data"));return;}
    std::memcpy(target,text.c_str(),bytes);GlobalUnlock(memory);
    if(!OpenClipboard(m_card)){GlobalFree(memory);status(tr(L"剪贴板忙，请稍后重试",L"Clipboard busy; retry shortly"));return;}
    const bool ok=EmptyClipboard()&&SetClipboardData(CF_UNICODETEXT,memory);
    CloseClipboard();if(!ok)GlobalFree(memory);
    status(ok?tr(L"已复制",L"Copied"):tr(L"复制失败，请重试",L"Copy failed; retry"));
}
void Window::reveal() {
    if(!m_selection.locked||!m_selection.locked->hasPath()||!m_pendingTool.empty())return;
    const auto& path=m_selection.locked->path;
    // The host validates the path; avoid filesystem I/O on this UI thread.
    const auto generation=++m_toolGeneration;
    m_toolStarted=GetTickCount64();
    status(tr(L"正在定位文件…",L"Revealing file…"));
    m_pendingTool=m_tool("show_in_folder",winshun::Json::Object{{"path",utf8(path)}},[this,generation](winshun::Json result){
        post([this,generation,result]{
            if(generation!=m_toolGeneration)return;
            m_pendingTool.clear();updateCard();
            if(result.has("error")) status(tr(L"无法定位文件：",L"Could not reveal: ")+wide(result["error"]["message"].toString()));
            else status(tr(L"已请求在文件夹中定位",L"Requested reveal in folder"));
        });
    });
    if(m_pendingTool.empty())status(tr(L"宿主工具不可用",L"Host tool unavailable"));
    updateCard();
}

void Window::drawCard(HDC dc) {
    RECT client{};GetClientRect(m_card,&client);FillRect(dc,&client,m_background);
    Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    const auto box=[this](int x,int y,int w,int h){return Gdiplus::RectF(static_cast<float>(px(x)),static_cast<float>(px(y)),static_cast<float>(px(w)),static_cast<float>(px(h)));};
    Gdiplus::Pen divider(ink(blend(m_look.background,m_look.border,65)),1);
    g.DrawLine(&divider,static_cast<float>(px(24)),static_cast<float>(px(52)),static_cast<float>(px(456)),static_cast<float>(px(52)));
    glyph(g,Pick,static_cast<float>(px(24)),static_cast<float>(px(18)),static_cast<float>(px(19)),m_look.accent);
    const bool ready=m_selection.locked&&m_selection.phase!=Phase::Resolving;
    const int radius=m_look.rounded?std::max(6,m_look.radius):0;
    if(ready) {
        const auto tint=blend(m_look.background,m_look.accent,m_look.dark?13:7);
        roundBox(g,box(24,76,44,44),static_cast<float>(px(radius+3)),tint,tint,false);
        glyph(g,0,static_cast<float>(px(34)),static_cast<float>(px(86)),static_cast<float>(px(24)),m_look.accent);
        roundBox(g,box(24,174,432,88),static_cast<float>(px(radius)),m_softColor,m_softColor,false);
        g.DrawLine(&divider,static_cast<float>(px(24)),static_cast<float>(px(footerY())),static_cast<float>(px(456)),static_cast<float>(px(footerY())));
        if(m_details)roundBox(g,box(24,footerY()+54,432,100),static_cast<float>(px(radius)),m_softColor,m_softColor,false);
    } else {
        const auto tint=blend(m_look.background,m_look.accent,m_look.dark?12:6);
        roundBox(g,box(214,82,52,52),static_cast<float>(px(m_look.rounded?16:0)),tint,tint,false);
        glyph(g,Pick,static_cast<float>(px(226)),static_cast<float>(px(94)),static_cast<float>(px(28)),m_look.accent);
    }
    if(hasStatus())g.DrawLine(&divider,static_cast<float>(px(24)),static_cast<float>(px(cardHeight()-(ready?52:44))),static_cast<float>(px(456)),static_cast<float>(px(cardHeight()-(ready?52:44))));
}
void Window::drawButton(const DRAWITEMSTRUCT& item) {
    const int id=static_cast<int>(item.CtlID);
    const bool primary=id==Copy||(id==Pick&&!m_selection.locked);
    const bool disabled=(item.itemState&ODS_DISABLED)!=0,hover=m_hover==item.hwndItem&&!disabled,pressed=(item.itemState&ODS_SELECTED)!=0;
    const bool quiet=id==Details||id==Diagnostics||id==Close||(id==Pick&&m_selection.locked);
    RECT rect=item.rcItem;FillRect(item.hDC,&rect,m_background);
    COLORREF fill=disabled?m_softColor:primary?m_look.accentFill:quiet?m_look.background:m_look.panel;
    COLORREF text=disabled?m_look.secondary:primary?m_look.accentText:quiet?m_look.secondary:m_look.text;
    if(hover||pressed){fill=blend(fill,primary?m_look.accentText:m_look.text,pressed?12:6);if(quiet)text=m_look.text;}
    Gdiplus::Graphics g(item.hDC);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Gdiplus::RectF bounds(.5f,.5f,static_cast<float>(rect.right-1),static_cast<float>(rect.bottom-1));
    roundBox(g,bounds,static_cast<float>(px(m_look.rounded?std::max(6,m_look.radius):0)),fill,primary&&!disabled?fill:m_look.border,!quiet&&!disabled);
    if((item.itemState&ODS_FOCUS)!=0) {
        Gdiplus::RectF ring(static_cast<float>(px(3)),static_cast<float>(px(3)),static_cast<float>(rect.right-px(6)),static_cast<float>(rect.bottom-px(6)));
        roundBox(g,ring,static_cast<float>(px(m_look.rounded?4:0)),fill,m_look.accent);
    }
    wchar_t label[160]{};GetWindowTextW(item.hwndItem,label,160);
    const int saved=SaveDC(item.hDC);SelectObject(item.hDC,m_font);SetBkMode(item.hDC,TRANSPARENT);SetTextColor(item.hDC,text);
    SIZE measured{};GetTextExtentPoint32W(item.hDC,label,static_cast<int>(wcslen(label)),&measured);
    const int icon=px(id==Close?20:17),gap=px(8);
    int left=(rect.right-(icon+gap+measured.cx))/2;
    if(quiet&&id!=Pick)left=px(2);
    if(id==Close)left=(rect.right-icon)/2;
    const int offset=pressed?px(1):0;
    glyph(g,id,static_cast<float>(left+offset),static_cast<float>((rect.bottom-icon)/2+offset),static_cast<float>(icon),text,m_details);
    if(id!=Close) {
        RECT labelRect{left+icon+gap+offset,offset,rect.right-px(2),rect.bottom+offset};
        DrawTextW(item.hDC,label,-1,&labelRect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    }
    RestoreDC(item.hDC,saved);
}
void Window::drawHint(HWND hwnd,HDC dc) {
    RECT rect{};GetClientRect(hwnd,&rect);FillRect(dc,&rect,m_background);
    const HPEN border=CreatePen(PS_SOLID,1,m_look.accent);const auto old=SelectObject(dc,border);
    const auto brush=SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,0,0,rect.right,rect.bottom);SelectObject(dc,brush);SelectObject(dc,old);DeleteObject(border);
    SetBkMode(dc,TRANSPARENT);SelectObject(dc,m_font);SetTextColor(dc,m_look.text);
    RECT line{px(12),px(9),rect.right-px(12),px(32)};
    std::wstring text=tr(L"指向一个窗口",L"Point at a window");
    if(m_selection.preview)text=m_selection.preview->hasPath()?m_selection.preview->executable():systemError(m_selection.preview->error,m_look.english);
    else if(m_selection.candidate)text=tr(L"正在识别窗口…",L"Identifying window…");
    DrawTextW(dc,text.c_str(),-1,&line,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    line.top=px(41);line.bottom=px(68);SetTextColor(dc,m_look.secondary);
    text=m_drag?tr(L"松开锁定 · Esc 取消",L"Release to lock · Esc to cancel"):tr(L"单击锁定 · Esc 取消",L"Click to lock · Esc to cancel");
    DrawTextW(dc,text.c_str(),-1,&line,DT_SINGLELINE|DT_NOPREFIX);
}

LRESULT CALLBACK Window::hoverButton(HWND hwnd,UINT msg,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR ref) {
    auto& self=*reinterpret_cast<Window*>(ref);
    if(msg==WM_MOUSEMOVE&&self.m_hover!=hwnd) {
        if(self.m_hover)InvalidateRect(self.m_hover,nullptr,FALSE);
        self.m_hover=hwnd;InvalidateRect(hwnd,nullptr,FALSE);
        TRACKMOUSEEVENT track{sizeof track,TME_LEAVE,hwnd,0};TrackMouseEvent(&track);
    }
    if(msg==WM_MOUSELEAVE&&self.m_hover==hwnd){self.m_hover=nullptr;InvalidateRect(hwnd,nullptr,FALSE);}
    if(msg==WM_SETCURSOR&&IsWindowEnabled(hwnd)){SetCursor(LoadCursorW(nullptr,IDC_HAND));return TRUE;}
    return DefSubclassProc(hwnd,msg,w,l);
}
LRESULT CALLBACK Window::pickButton(HWND hwnd,UINT msg,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR ref) {
    auto& self=*reinterpret_cast<Window*>(ref);
    if(msg==WM_LBUTTONDOWN){self.m_buttonDown=true;GetCursorPos(&self.m_buttonOrigin);SetCapture(hwnd);SendMessageW(hwnd,BM_SETSTATE,TRUE,0);return 0;}
    if(msg==WM_MOUSEMOVE&&self.m_buttonDown){POINT point{};GetCursorPos(&point);
        if(std::abs(point.x-self.m_buttonOrigin.x)+std::abs(point.y-self.m_buttonOrigin.y)>self.px(7)){
            self.m_buttonDown=false;SendMessageW(hwnd,BM_SETSTATE,FALSE,0);self.begin(true);
        }return 0;}
    if(msg==WM_LBUTTONUP&&self.m_buttonDown){self.m_buttonDown=false;ReleaseCapture();SendMessageW(hwnd,BM_SETSTATE,FALSE,0);self.begin(false);return 0;}
    if(msg==WM_CAPTURECHANGED){self.m_buttonDown=false;SendMessageW(hwnd,BM_SETSTATE,FALSE,0);}
    return DefSubclassProc(hwnd,msg,w,l);
}
LRESULT CALLBACK Window::procedure(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    Window* self=reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){self=static_cast<Window*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    return self?self->message(hwnd,msg,w,l):DefWindowProcW(hwnd,msg,w,l);
}
LRESULT Window::message(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
    if(msg==Dispatch){drain();return 0;}
    if(msg==Stop){endInput();if(!m_pendingTool.empty())m_cancelTool(m_pendingTool);PostQuitMessage(0);return 0;}
    if(msg==WM_TIMER&&hwnd==m_controller){
        if(w==3){KillTimer(m_controller,3);begin();}
        if(w==1)tick();
        if(w==2){
            if(m_selection.locked&&!m_selection.locked->stale&&!stillSame(*m_selection.locked)){
                m_selection.locked->stale=true;{const std::lock_guard lock(m_mutex);m_snapshot=m_selection.locked;}updateCard();
            }
            if(!m_pendingTool.empty()&&GetTickCount64()-m_toolStarted>5000){m_cancelTool(m_pendingTool);m_pendingTool.clear();++m_toolGeneration;updateCard();status(tr(L"定位请求超时",L"Reveal request timed out"));}
        }return 0;
    }
    const bool shield=std::find(m_shields.begin(),m_shields.end(),hwnd)!=m_shields.end();
    if(shield&&m_picking){
        if(msg==WM_SETCURSOR){SetCursor(LoadCursorW(nullptr,IDC_CROSS));return TRUE;}
        if(msg==WM_LBUTTONDOWN){POINT point{};GetCursorPos(&point);m_selection.point(identify(hitWindow(point,GetCurrentProcessId())));m_down=m_selection.candidate;m_armed=true;SetCapture(hwnd);return 0;}
        if(msg==WM_LBUTTONUP){if(m_drag){POINT point{};GetCursorPos(&point);m_selection.point(identify(hitWindow(point,GetCurrentProcessId())));}if(m_armed&&m_selection.candidate)confirm();else{m_armed=false;ReleaseCapture();}return 0;}
        if(msg==WM_RBUTTONDOWN||msg==WM_RBUTTONUP)return 0;
        if(msg==WM_KEYDOWN&&w==VK_ESCAPE){cancel();return 0;}
        if(msg==WM_PAINT){PAINTSTRUCT ps{};const auto dc=BeginPaint(hwnd,&ps);FillRect(dc,&ps.rcPaint,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));EndPaint(hwnd,&ps);return 0;}
    }
    if(msg==WM_PAINT&&hwnd==m_hint){PAINTSTRUCT ps{};auto dc=BeginPaint(hwnd,&ps);drawHint(hwnd,dc);EndPaint(hwnd,&ps);return 0;}
    if(msg==WM_PAINT&&std::find(std::begin(m_edges),std::end(m_edges),hwnd)!=std::end(m_edges)){
        PAINTSTRUCT ps{};auto dc=BeginPaint(hwnd,&ps);const auto brush=CreateSolidBrush(m_look.accent);FillRect(dc,&ps.rcPaint,brush);DeleteObject(brush);EndPaint(hwnd,&ps);return 0;
    }
    if(hwnd==m_card){
        if(msg==WM_NCCALCSIZE&&w)return 0;
        if(msg==WM_NCHITTEST){POINT point{GET_X_LPARAM(l),GET_Y_LPARAM(l)};ScreenToClient(hwnd,&point);return point.y<px(52)&&point.x<px(424)?HTCAPTION:HTCLIENT;}
        if(msg==WM_NCLBUTTONDBLCLK&&w==HTCAPTION)return 0;
        if(msg==WM_PAINT) {
            PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);RECT rect{};GetClientRect(hwnd,&rect);
            HDC memory=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,rect.right,rect.bottom);
            if(memory&&bitmap){auto old=SelectObject(memory,bitmap);drawCard(memory);BitBlt(dc,0,0,rect.right,rect.bottom,memory,0,0,SRCCOPY);SelectObject(memory,old);}
            else drawCard(dc);
            if(bitmap)DeleteObject(bitmap);
            if(memory)DeleteDC(memory);
            EndPaint(hwnd,&ps);return 0;
        }
        if(msg==WM_COMMAND){switch(LOWORD(w)){
            case Pick:begin();break;case Copy:copy(false);break;case Reveal:reveal();break;
            case Details:m_details=!m_details;layout();updateCard();break;case Diagnostics:copy(true);break;
            case Close:SendMessageW(hwnd,WM_CLOSE,0,0);break;
        }return 0;}
        if(msg==WM_CLOSE){if(m_selection.phase==Phase::Resolving)m_selection.cancel();ShowWindow(hwnd,SW_HIDE);KillTimer(m_controller,2);return 0;}
        if(msg==WM_DPICHANGED){m_dpi=HIWORD(w);const auto rect=reinterpret_cast<RECT*>(l);SetWindowPos(hwnd,nullptr,rect->left,rect->top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);applyLook();return 0;}
        if(msg==WM_DRAWITEM){drawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(l));return TRUE;}
        if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLOREDIT){auto dc=reinterpret_cast<HDC>(w);const int id=GetDlgCtrlID(reinterpret_cast<HWND>(l));
            SetTextColor(dc,id==Note||id==Status||id==Meta||id==Title||id==PathLabel?m_look.secondary:m_look.text);
            const bool soft=id==Path||id==Technical||id==PathLabel;
            SetBkColor(dc,soft?m_softColor:m_look.background);return reinterpret_cast<LRESULT>(soft?m_soft:m_background);}
        if(msg==WM_ERASEBKGND)return 1;
    }
    return DefWindowProcW(hwnd,msg,w,l);
}
void Window::run() {
    Gdiplus::GdiplusStartupInput graphics;
    if(Gdiplus::GdiplusStartup(&m_gdiplus,&graphics,nullptr)!=Gdiplus::Ok){const std::lock_guard lock(m_mutex);m_ready=true;m_readyCondition.notify_all();return;}
    WNDCLASSEXW cls{};cls.cbSize=sizeof cls;cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=ClassName;cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    RegisterClassExW(&cls);
    HWND controller=CreateWindowExW(0,ClassName,L"WhatIsController",0,0,0,0,0,HWND_MESSAGE,nullptr,cls.hInstance,this);
    m_card=CreateWindowExW(WS_EX_TOPMOST,ClassName,L"这是什么 · Win顺",WS_POPUP|WS_THICKFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,480,388,nullptr,nullptr,cls.hInstance,this);
    if(controller&&m_card){
        m_dpi=GetDpiForWindow(m_card);
        for(int id:{Heading,Pick,Exe,PathLabel,Path,Copy,Reveal,Title,Meta,Note,Details,Technical,Diagnostics,Status,Close}){
            const bool button=id>=100&&id<200,edit=id==Path||id==Title||id==Technical;
            DWORD style=WS_CHILD|WS_VISIBLE;
            if(button)style|=WS_TABSTOP|BS_OWNERDRAW;
            else if(edit)style|=WS_TABSTOP|ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL;
            else style|=SS_NOPREFIX;
            CreateWindowExW(0,button?L"BUTTON":edit?L"EDIT":L"STATIC",L"",style,0,0,10,10,m_card,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),cls.hInstance,nullptr);
            if(button)SetWindowSubclass(control(id),hoverButton,2,reinterpret_cast<DWORD_PTR>(this));
        }
        SetWindowSubclass(control(Pick),pickButton,1,reinterpret_cast<DWORD_PTR>(this));applyLook();updateCard();
    }
    {const std::lock_guard lock(m_mutex);m_controller=controller&&m_card?controller:nullptr;m_ready=true;}
    m_readyCondition.notify_all();
    if(controller&&m_card){
        MSG msg{};
        while(GetMessageW(&msg,nullptr,0,0)>0){
            if(msg.message==WM_KEYDOWN&&msg.wParam==VK_ESCAPE){
                if(m_picking)cancel();else if(IsWindowVisible(m_card))SendMessageW(m_card,WM_CLOSE,0,0);continue;
            }
            if(!m_picking&&IsWindowVisible(m_card)&&IsDialogMessageW(m_card,&msg))continue;
            TranslateMessage(&msg);DispatchMessageW(&msg);
        }
    }
    endInput();
    {const std::lock_guard lock(m_mutex);m_controller=nullptr;m_queue.clear();}
    if(m_card)DestroyWindow(m_card);
    m_card=nullptr;
    if(controller)DestroyWindow(controller);
    clearGraphics();
    Gdiplus::GdiplusShutdown(m_gdiplus);m_gdiplus=0;
}
}
