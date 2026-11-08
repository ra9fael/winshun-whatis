#include "window_probe.h"
#include <dwmapi.h>
#include <vector>

namespace whatis {
namespace {
struct Handle {
    HANDLE value;
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
bool externalVisible(HWND hwnd, DWORD exclude, POINT point) {
    const auto id = identify(hwnd);
    if (!id || id.pid == exclude || !IsWindowVisible(hwnd)) return false;
    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof cloaked)) && cloaked) return false;
    RECT bounds{};
    if (!GetWindowRect(hwnd, &bounds) || !PtInRect(&bounds, point)) return false;
    if ((GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & (WS_EX_LAYERED | WS_EX_TRANSPARENT)) == (WS_EX_LAYERED | WS_EX_TRANSPARENT)) return false;
    const HRGN region = CreateRectRgn(0, 0, 0, 0);
    if (region) {
        const int kind = GetWindowRgn(hwnd, region);
        const bool outside = kind != ERROR && !PtInRegion(region, point.x - bounds.left, point.y - bounds.top);
        DeleteObject(region);
        if (outside) return false;
    }
    return true;
}
std::uint64_t creation(HANDLE process) {
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(process, &created, &exited, &kernel, &user)) return 0;
    return (static_cast<std::uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
}
}
std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), n, nullptr, nullptr);
    return out;
}
std::wstring wide(const std::string& value) {
    if (value.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), n);
    return out;
}
Identity identify(HWND hwnd) {
    DWORD pid = 0;
    if (!GetWindowThreadProcessId(hwnd, &pid)) return {};
    return {reinterpret_cast<std::uint64_t>(hwnd), pid};
}
HWND hitWindow(POINT point, DWORD excludedPid) {
    struct Search { POINT point; DWORD excluded; HWND found = nullptr; } search{point, excludedPid};
    // Enumerate z-order below our input shields instead of temporarily hiding
    // the shield (which would expose a click-through race).
    EnumWindows([](HWND hwnd, LPARAM data) -> BOOL {
        auto& state = *reinterpret_cast<Search*>(data);
        if (!externalVisible(hwnd, state.excluded, state.point)) return TRUE;
        wchar_t cls[128]{};
        GetClassNameW(hwnd, cls, 128);
        if (std::wstring(cls) == L"Progman" || std::wstring(cls) == L"WorkerW") return TRUE;
        state.found = hwnd;
        return FALSE;
    }, reinterpret_cast<LPARAM>(&search));
    HWND current = search.found;
    for (unsigned depth = 0; current && depth < 32; ++depth) {
        POINT local = point;
        if (!ScreenToClient(current, &local)) break;
        const HWND child = ChildWindowFromPointEx(current, local, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
        if (!child || child == current || identify(child).pid == excludedPid) break;
        current = child;
    }
    // Same-process child controls belong to their root window for this product.
    // Cross-process children retain their own PID rather than being relabeled.
    if (current && identify(current).pid == identify(search.found).pid) return search.found;
    return current;
}
RECT frameRect(HWND hwnd) {
    RECT bounds{};
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &bounds, sizeof bounds)) || IsRectEmpty(&bounds)) GetWindowRect(hwnd, &bounds);
    return bounds;
}
Result inspect(Identity identity) {
    Result out;
    out.identity = identity;
    const HWND hwnd = reinterpret_cast<HWND>(identity.hwnd);
    if (!identity || identify(hwnd) != identity) { out.error = ERROR_INVALID_WINDOW_HANDLE; out.stale = true; return out; }
    const HWND root = GetAncestor(hwnd, GA_ROOT);
    out.root = reinterpret_cast<std::uint64_t>(root);
    out.rootPid = identify(root).pid;
    wchar_t title[2048]{}, cls[512]{};
    // Cross-process GetWindowText reads the cached caption, not WM_GETTEXT in
    // the target process. Excluding our own windows avoids its same-process path.
    if (identity.pid != GetCurrentProcessId()) GetWindowTextW(hwnd, title, 2048);
    GetClassNameW(hwnd, cls, 512);
    out.title = title;
    out.windowClass = cls;
    Handle process{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, identity.pid)};
    if (!process.value) out.error = GetLastError();
    else {
        out.created = creation(process.value);
        for (DWORD size = 512; size <= 65536; size *= 2) {
            std::vector<wchar_t> buffer(size);
            DWORD length = size;
            if (QueryFullProcessImageNameW(process.value, 0, buffer.data(), &length)) { out.path.assign(buffer.data(), length); out.error = 0; break; }
            out.error = GetLastError();
            if (out.error != ERROR_INSUFFICIENT_BUFFER) break;
        }
    }
    if (identify(hwnd) != identity) { out.stale = true; out.path.clear(); out.error = ERROR_INVALID_WINDOW_HANDLE; }
    return out;
}
bool stillSame(const Result& result) {
    if (identify(reinterpret_cast<HWND>(result.identity.hwnd)) != result.identity) return false;
    if (!result.created) return true;
    Handle process{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, result.identity.pid)};
    // Loss of read permission does not prove that a process exited.
    return !process.value || creation(process.value) == result.created;
}
bool inputDesktopAvailable() {
    HDESK input = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    if (!input) return false;
    wchar_t active[256]{}, own[256]{}; DWORD bytes = 0;
    const bool ok = GetUserObjectInformationW(input, UOI_NAME, active, sizeof active, &bytes) &&
        GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()), UOI_NAME, own, sizeof own, &bytes) &&
        std::wstring(active) == own;
    CloseDesktop(input);
    return ok;
}
void enableDpiAwareness() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}
}
