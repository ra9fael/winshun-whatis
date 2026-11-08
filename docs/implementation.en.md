# Native implementation

[中文](implementation.md)

## Verdict

Identify the window-creating process before discussing application ownership. Unreadable paths stay unknown; do not infer a content app from a parent process, filename or page title. Implementation began after the user approved the preceding design and HTML prototype on November 8, 2026.

## Structure

- `src/core/selection.h`: Windows-independent state, generations, candidate identity and results.
- `src/platform/window_probe.*`: hit-testing, HWND/PID, title, class, root window, limited-access process path and creation time.
- `src/ui/window.*`: a dedicated UI thread, one query worker, native controls, input surfaces, borders, hints and result card.
- `src/main.cpp`: stdio JSON-RPC and diagnostic CLI; queries open no UI and actions promptly return the launcher hide effect.
- `winshun.h`: vendored host SDK with a thread-safe asynchronous tool extension; see attribution.

Queries carry a monotonic generation and HWND/PID. Cancellation, candidate changes and confirmation invalidate old responses. Confirmation queries again; creation time helps detect PID reuse after locking. Cross-process captions use cached window text rather than unbounded `WM_GETTEXT` messages.

## Adjustments from the prototype

### November 8, 2026 UI revision

The user confirmed functionality and requested a visual improvement. This revision implements the intended compact result card: unified caption, program glyph and EXE heading, a distinct path surface, grouped primary/secondary actions, and a footer for details and repicking. Chinese uses Microsoft YaHei UI; English uses Segoe UI. Graphics are antialiased; native buttons retain keyboard/accessibility semantics with hover and focus feedback. The empty state hides irrelevant fields, routine ownership guidance moves into details, and errors stay visible. Existing permissions, inspection and selection logic remain in use. Verification covers light/dark, both languages, empty/denied screenshots, existing integration checks and real-host roundtrips.

1. Picking creates one temporary alpha=1 layered input window per monitor. These receive button down/up; borders and hints are nonactivating and transparent to input. Z-order enumeration finds windows underneath, excluding our own windows, hidden/cloaked windows, desktop surfaces and obvious transparent layers. Region/child checks preserve cross-process child PIDs and normalize same-PID controls to the root. This does not reproduce every application's custom hit-testing.
2. No global mouse hook is installed. Dragging uses capture. Shutdown, cancellation, foreground loss, desktop changes and the 30-second timeout remove surfaces and capture. Physical isolation still needs manual acceptance.
3. The host action path does not grant foreground permission. Picking starts after 120 ms; if Windows denies activation, a topmost card asks the user to click the picker. Host code was not changed.
4. Files are consolidated into core, platform and UI components, with a single monotonic generation representing sessions. The MVP does not store an owner chain, sampling timestamp or separate content-app identity.
5. `pick` is the only host command action. Copy/details are native control actions. Reveal uses `show_in_folder`, times out after 5 seconds and ignores late replies. The host checks file validity off-thread; the plugin UI performs no synchronous filesystem check.
6. The initial view reuses the card's empty state and picker entry; advanced details are collapsed. The prototype's simulated Explorer is illustrative; native reveal belongs to the host.

## Failure behavior

| Condition | Behavior |
|---|---|
| Path access denied | Preserve HWND/PID/error, disable path actions |
| Closed window | Mark historical snapshot; discard path if identity changes during inspection |
| ApplicationFrameHost / RuntimeBroker | Show the verified host EXE; content app remains unknown |
| Copy failure | Report error; path remains selectable |
| Reveal error / timeout | Show host error or timeout and restore the button |
| Foreground loss / desktop switch | Cancel, remove input surfaces and show the card |
| No candidate | Wait for a valid window or Escape |

## Verification and limits

Measured: real host loading and tool roundtrips work without host changes. The normal sandbox can inspect the fixture EXE; a real DACL denial degrades correctly. The system hit-tests the input surface and directed messages complete selection. CTest 2/2, 31 integration and 12 real-host checks passed.

Inferred: temporary surfaces should consume ordinary desktop confirmation clicks, creation times help detect PID reuse, and a native architecture should limit resource cost. These are not physical-input, stress-reuse or performance acceptance results.

See [validation](validation.en.md) for the complete L0–L5 mapping and outstanding manual checks.
