# What Is This prototype validation

[中文](README.md)

Date: 2026-11-08. Scope: proposal, standalone HTML, and inline interaction fragment. The native plugin is not implemented.

## 1. Problem

Validate the proposed Winshun flow of selecting a window, identifying its EXE, and copying or revealing its path. Cover ordinary windows, unfamiliar popups, host windows, and unreadable paths. Desktop and narrow layouts must remain usable, and simulated data must be clearly identified.

Environment: Windows; Playwright controlling an independent headless Microsoft Edge `156.0.4314.58`. Local main repository HEAD: `1769c368`. The user's running Winshun instance was not started or modified.

## 2. Cause

Windows and applications do not always have a one-to-one relationship, and path queries can fail. The prototype therefore needs idle, preview, locked, and partial-result states, plus an explicit unresolved content-app state for hosts.

Cancellation also needs to handle late input: releasing a crosshair drag after Escape must not commit the canceled selection. Changing samples must close an action preview belonging to the previous result.

## 3. Changes

- Added `whatis-plugin-design.md`, `whatis-plugin-design.en.md`, and `whatis-plugin-demo.html` at the workspace root.
- Added the approximately 26 KB inline source at `C:/Users/renfei/.codex/visualizations/2026/11/08/01a1ba5a-cd30-7af1-a610-ed213c0bb93c/whatis-window.html`.
- Implemented click/drag selection, keyboard operation, cancellation fallback, manual clipboard fallback, simulated file reveal, collapsible details, light/dark appearance, and narrow layouts.
- Executables, paths, PIDs, HWNDs, and application names are fixed examples. Reveal opens only a simulated Explorer preview.

## 4. Measured

Machine-readable evidence: [checks.json](checks.json). **23 of 23 checks passed**, with no page JavaScript runtime errors.

| Check | Expected | Actual / evidence |
|---|---|---|
| Start, hover, and lock | Preview actions disabled; locked actions enabled | Passed; checks 1–4 |
| Simulated reveal | Directory and selected EXE shown without launching a system program | Passed; check 5 |
| Clipboard rejection | Selectable manual fallback without false success | Passed; check 6 |
| Escape, drag, and details | Restore old result; release locks; details accessible | Passed; checks 7–9 |
| Denied access and hosts | Disable unavailable actions, retain PID, identify host uncertainty | Passed; checks 10–12 |
| Keyboard and canceled drag | Enter starts, Space locks, release after Escape cannot commit | Passed; checks 13–15 |
| Successful copy branch | Success feedback only after the write promise resolves | Injected adapter; check 16 |
| 320–1140 px | No horizontal overflow | Passed at 1140, 1024, 760, 600, 375, and 320; check 17 |
| Runtime | No script errors | Passed; check 18 |
| Selection changes | Close the previous action preview | Passed; check 19 |
| Sandboxed iframe | Fragment works and clipboard restrictions have a manual fallback | Passed; checks 20–21 |
| Syntax and documents | Script parses; bilingual links and 11 required sections exist | Passed; checks 22–23 |

The following screenshots were visually inspected for overlap and clipping in light, dark, and 320 px layouts. They were captured before the final behavior-only fix that closes an old action preview when changing selection; their displayed layouts were unchanged by that fix. Check 19 covers the final behavior.

- [Desktop light](desktop-light.png)
- [Desktop dark](desktop-dark.png)
- [320 px narrow layout](mobile-320.png)

Copy success/rejection used injected adapters; **the user's system clipboard was not modified**. The sandbox manual fallback was checked separately. The independent browser and contexts were closed. No server, build directory, or temporary user configuration was left running or retained.

Final fragment and standalone SHA256 hashes are in checks.json. Only this validation directory was added to the main repository. The existing envhosts-native and findmouse-presets validation directories were preserved. The workspace root is not a Git repository, so the design and HTML are uncommitted files. No commit, publication, or plugin installation was performed.

## 5. Not tested

- Real Windows enumeration, HWND/PID/EXE queries, and native input capture.
- Administrative/protected processes, cross-session access, secure desktops, content-app resolution, and physical mixed-DPI monitors.
- Native performance, memory, handle leaks, installation, and real Winshun host round trips.
- Actual system clipboard writes, real Explorer reveal, physical touch input, and screen readers.
- Product-host interaction-state persistence and Tweak controls were not tested inside the product; standalone and sandbox behavior without host APIs was verified.

These limits do not prevent using the HTML as an interaction prototype, but the checks do not establish native plugin readiness.
