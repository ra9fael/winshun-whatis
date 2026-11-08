#pragma once
#include "src/platform/window_probe.h"
#include "winshun.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace whatis {
struct Look {
    bool english = false, dark = false, rounded = true;
    float scale = 1;
    int radius = 6;
    COLORREF background = RGB(246,247,250), panel = RGB(255,255,255), text = RGB(32,38,52),
        secondary = RGB(101,112,131), border = RGB(220,225,233), accent = RGB(36,99,184),
        accentFill = RGB(33,98,181), accentText = RGB(255,255,255);
    static Look from(const winshun::Json& appearance, const std::string& theme, bool english);
};

class Window {
public:
    using Tool = std::function<std::string(const std::string&, const winshun::Json&, std::function<void(winshun::Json)>)>;
    using CancelTool = std::function<void(const std::string&)>;
    Window(Tool tool, CancelTool cancel);
    ~Window();
    bool start();
    void stop();
    bool pick();
    void appearance(Look look);
    std::optional<Result> snapshot() const;
    bool picking() const { return m_picking.load(); }
private:
    static LRESULT CALLBACK procedure(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK pickButton(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    static LRESULT CALLBACK hoverButton(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    LRESULT message(HWND, UINT, WPARAM, LPARAM);
    void run();
    bool post(std::function<void()> task);
    void drain();
    void probeLoop();
    void requestProbe();
    void begin(bool drag = false);
    void endInput();
    void cancel(const std::wstring& reason = {});
    void confirm();
    void tick();
    void showCard();
    void updateCard();
    void layout();
    void applyLook();
    void updateIcons();
    void clearGraphics();
    void drawButton(const DRAWITEMSTRUCT& item);
    void drawCard(HDC dc);
    bool hasNotice() const;
    bool hasStatus() const;
    int footerY() const;
    int cardHeight() const;
    void drawHint(HWND hwnd, HDC dc);
    void updateHighlight(POINT point);
    void copy(bool diagnostics);
    void reveal();
    void status(std::wstring text);
    std::wstring tr(const wchar_t* zh, const wchar_t* en) const { return m_look.english ? en : zh; }
    int px(int value) const;
    HWND control(int id) const { return GetDlgItem(m_card, id); }
    void set(int id, const std::wstring& text) { SetWindowTextW(control(id), text.c_str()); }
    void move(int id, int x, int y, int w, int h, bool visible = true);
    std::thread m_thread, m_worker;
    mutable std::mutex m_mutex;
    std::condition_variable m_readyCondition, m_probeCondition;
    std::deque<std::function<void()>> m_queue;
    std::optional<Ticket> m_work;
    std::optional<Result> m_snapshot;
    bool m_ready = false, m_stopping = false, m_workerStop = false;
    HWND m_controller = nullptr, m_card = nullptr, m_hint = nullptr;
    HICON m_icon = nullptr, m_smallIcon = nullptr, m_captionIcon = nullptr;
    std::vector<HWND> m_shields;
    HWND m_edges[4]{};
    std::atomic<bool> m_picking{false};
    Selection m_selection;
    Look m_look;
    Tool m_tool;
    CancelTool m_cancelTool;
    std::string m_pendingTool;
    std::uint64_t m_toolGeneration = 0;
    ULONGLONG m_toolStarted = 0, m_started = 0, m_candidateSince = 0;
    Identity m_down;
    bool m_armed = false, m_drag = false, m_details = false, m_probeQueued = false;
    bool m_buttonDown = false;
    POINT m_buttonOrigin{};
    UINT m_dpi = 96;
    HFONT m_font = nullptr, m_large = nullptr, m_small = nullptr, m_heading = nullptr, m_mono = nullptr;
    HBRUSH m_background = nullptr, m_panel = nullptr, m_soft = nullptr;
    COLORREF m_softColor = RGB(242,244,247);
    HWND m_hover = nullptr;
    ULONG_PTR m_gdiplus = 0;
    std::wstring m_status;
};
winshun::Json resultJson(const Result& result);
std::wstring diagnosticText(const Result& result, bool english);
}
