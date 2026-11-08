#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

namespace whatis {
struct Identity {
    std::uint64_t hwnd = 0;
    std::uint32_t pid = 0;
    bool operator==(const Identity& other) const { return hwnd == other.hwnd && pid == other.pid; }
    bool operator!=(const Identity& other) const { return !(*this == other); }
    explicit operator bool() const { return hwnd != 0 && pid != 0; }
};
struct Result {
    Identity identity;
    std::uint64_t root = 0;
    std::uint32_t rootPid = 0;
    std::uint64_t created = 0;
    std::uint32_t error = 0;
    std::wstring title, windowClass, path;
    bool stale = false;
    std::wstring executable() const {
        const auto slash = path.find_last_of(L"\\/");
        return slash == std::wstring::npos ? path : path.substr(slash + 1);
    }
    bool knownHost() const {
        auto file = executable();
        std::transform(file.begin(), file.end(), file.begin(), [](wchar_t c) {
            return c >= L'A' && c <= L'Z' ? static_cast<wchar_t>(c + L'a' - L'A') : c;
        });
        return file == L"applicationframehost.exe" || file == L"runtimebroker.exe";
    }
    bool hasPath() const { return !path.empty() && error == 0; }
};
enum class Phase { Idle, Picking, Resolving, Locked };
struct Ticket { std::uint64_t generation; Identity identity; };

// The state machine contains no Windows headers and rejects responses from
// previous sessions/candidates, including a response arriving after Escape.
class Selection {
public:
    Phase phase = Phase::Idle;
    Identity candidate;
    std::optional<Result> preview, locked;
    std::uint64_t generation = 0;
    void begin() {
        ++generation;
        phase = Phase::Picking;
        candidate = {};
        preview.reset();
    }
    bool point(Identity next) {
        if (phase != Phase::Picking || next == candidate) return false;
        ++generation;
        candidate = next;
        preview.reset();
        return true;
    }
    Ticket ticket() const { return {generation, candidate}; }
    bool accept(Ticket ticket, Result result) {
        if ((phase != Phase::Picking && phase != Phase::Resolving) ||
            ticket.generation != generation || ticket.identity != candidate ||
            result.identity != candidate) return false;
        preview = std::move(result);
        if (phase == Phase::Resolving) {
            locked = preview;
            phase = Phase::Locked;
        }
        return true;
    }
    bool confirm() {
        if (phase != Phase::Picking || !candidate) return false;
        // Always issue a fresh query at confirmation, even if a preview exists.
        ++generation;
        preview.reset();
        phase = Phase::Resolving;
        return true;
    }
    void cancel() {
        ++generation;
        candidate = {};
        preview.reset();
        phase = locked ? Phase::Locked : Phase::Idle;
    }
};
}
