# What Is This UI revision verification

[中文](README.md)

## 1. Problem

The user confirmed functionality but found the interface unattractive. The original duplicated its system caption with an internal heading, scattered fields and actions, and presented the path as a large native edit area. Date: November 8, 2026. Windows x64, GCC 13.2, Winshun 0.22.0 development host.

## 2. Cause

The initial iteration prioritized Win32 inspection and host integration. Its linear control layout did not implement the prototype's hierarchy. Type, controls and spacing lacked consistent sizing, buttons had no hover feedback, and the empty state retained irrelevant fields.

## 3. Changes

Reorganized `ra9fael.whatis/src/ui/window.*` into a default 480 × 388 card: draggable custom caption, close button, generic program glyph, EXE heading, PID/state, window title, distinct path container, paired Copy/Reveal buttons and a Details/Repick footer. Routine ownership guidance moves inside details; exceptional notices remain visible. The empty state shows only guidance and the picker.

GDI+ antialiases icons and shapes. Native buttons and selectable text retain keyboard semantics, with hover/focus states and Chinese/English font selection. Host colors, corners and sizing remain supported. Permissions, window inspection, selection state and host source were not changed. The build adds the Windows system `gdiplus.dll` dependency.

Two test-driver fixes: drag steps now use synchronous directed messages with timeouts to avoid races with pointer movement; the host driver resolves `--out` and other paths before changing cwd.

## 4. Measured

- Final build succeeded without warnings. L0 schema, bilingual docs, archive contents/hash passed: `static.json`, `package.json`.
- 31 native UI/fake-host regressions passed: `integration.json`. Handles remained 175 across 12 cancellation cycles. Copy/reveal protocol, timeout, late replies, denial, dragging, target closure and shutdown checks remain covered.
- 12 production isolated-host checks passed: `host-smoke.json`. Launcher execution, actual fixture identification, reuse, actual host-tool error and process shutdown passed.
- 52 UI/layout checks passed: `visual.json`, covering empty-state visibility, caption hit region, default size, selectable full path, expansion, close, both languages/themes and compact/roomy/square layouts. A synthetic 150% DPI message produced a 720 px window width.
- Reviewed `after-light.png`, `after-dark.png`, `after-dark-en.png`, `after-empty.png`, `after-details.png`, `after-denied.png`, `after-roomy-square.png`, `after-150.png`. `after-host.png` shows the real host integration. `before.png` is the previous real-host capture. Default height fell from approximately 484 px to 388 px.
- EXE SHA256: `1eb4e39e901e0a4aebe74fe5c6f8f8ebf6e5049ec7ffa6fde8a3b66a67a30a57`.
- Updated development archive: `ra9fael.whatis/dist/ui-refresh/ra9fael.whatis-0.1.0-dev.wsplugin`; SHA256 `bfd2b26c95b8bc9a8b432f0be25513722e7ca7f63f1194bc111603430c07573c`. The earlier archive is preserved. Nothing was published or pushed.

Window checks use test-owned processes and directed messages. The pointer is restored and fixture/plugin processes stop. The successful host run's private profile was removed by the script. The clipboard and user's running Winshun were not modified. Raw build/capture diagnostics remain under plugin `build/ui-refresh/`; incorrectly relative preview output remains at `WinShun/build/debug/build/ui-refresh/host/`. Older test directories were not cleanup targets for this revision.

The first regression did not enter drag selection due to message timing; synchronous dispatch fixed the driver. The initial relative `--out` host run received no commands; absolute paths fixed it. That failed host was ended by the driver, which could not identify its private profile; no cleanup is claimed for it. These failures are not counted as passes.

## 5. Not tested

Physical caption dragging, DWM shadows/corners across Windows versions, real mixed-DPI monitors, high contrast and screen readers await manual acceptance. Synthetic DPI messaging verifies layout response only. Physical click isolation, clipboard and successful Explorer actions retain their prior unaccepted status; the user's functional feedback does not substitute for those specific checks. Core/platform code did not change, so the prior CTest 2/2 result was retained without rerunning it. Archive-import UI and publication were not exercised.
