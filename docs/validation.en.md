# Verification and manual acceptance

[中文](validation.md)

## 2026-11-09 window icon fix (development preview)

- Load the embedded named `IDI_ICON1`, assign large/small window icons and draw the same icon in the custom caption. Reload for DPI or UI size changes and release replaced resources.
- MSVC Release build succeeded; CTest passed 2/2. `tests/window_icons.py` passed 46/46: actual hidden-window pixels match the ICO at 96/120/144/192 DPI, GDI/USER counts remain 24/27 after 66 appearance updates, and the plugin exits cleanly.
- The installed WinShun host launched an isolated test plugin. Window handles, sizes and pixels passed; the captured caption displays the new icon.
- **Full host shutdown verification failed; no formal release was made.** Both the installed and local development hosts exit with `0xC0000005`. The original unmodified plugin reproduces the same error; a host without external plugins exits normally. This establishes that the shutdown failure was not introduced by the icon change; its root cause remains undiagnosed.
- The original full host scenario timed out while selecting a fixture window, so this run does not claim all previous interaction checks passed. Physical taskbar, Alt+Tab and cross-monitor DPI interaction remain unverified.
- The new development package is for local icon inspection and has not passed full release acceptance. Restart the plugin after importing it; closing a resident window may only hide it.
- Raw evidence is in the workspace at `docs/validation/plugin-window-icons-fix-20261109/`.

Date: November 8, 2026. The user tested and accepted the functionality and revised UI, then authorized a GitHub repository and `v0.1.0` tag. User feedback, automated results and coverage limits are distinguished below.

## 1. Problem

Verify Windows window-to-EXE lookup, Winshun protocol/permissions/lifecycle, native UI and standalone packaging. The host is a Winshun 0.22.0 development build (HEAD `1769c368`) on Windows x64. User acceptance and the full desktop checks refer to the GCC 13.2 build.

## 2. Cause

Processes may deny inspection, asynchronous replies may arrive late, and selection surfaces affect hit-testing and input. Successful synthetic global input calls do not establish physical behavior. During tag preparation, MSVC desktop revalidation could not complete: `SetCursorPos(248,180)` returned 0 and the actual position did not move. Repeating with the previously passing GCC binary encountered the same environment issue. A test precondition now prevents a wrong cursor position from being reported as a plugin regression.

## 3. Changes

See [implementation](implementation.en.md) for the plugin, state machine and native card. This iteration adds bilingual `0.1.0` notes, repository homepage, standalone CI, a pinned manifest schema, and version-specific bilingual package notes. The test fixture explicitly links `advapi32`. Product inspection, permission and interaction code is unchanged.

## 4. Measured

| Scope | Method and actual result |
|---|---|
| GCC / L1 and native basics | CTest 2/2 passed, including 22 pure core assertions |
| GCC / L2 and Win32 fixtures | 31/31: real DACL denial, async host success/failure/timeout/late replies, cancellation and shutdown |
| GCC / L3 real host | 12/12: installation, query/action, normal-sandbox inspection, reuse, missing-file tool error and shutdown |
| GCC / UI | 52/52: light/dark, both languages, empty/expanded/closed states, compact/roomy and synthetic 150% DPI layout |
| MSVC 19.51 / Release | Compilation and CTest 2/2 passed; desktop revalidation was limited by the cursor environment above and is not counted as passing |
| User acceptance | The user reported successful testing and approved the repository/tag; no device or edge-case checklist was supplied |

See [acceptance.json](verification/acceptance.json) for a machine-readable summary and binary fingerprints, and [screenshots](screenshots.en.md) for selected images. Original workspace evidence is under `docs/validation/whatis-ui-20261108/`, `whatis-native-20261108/` and this iteration's `whatis-tag-20261108/`. GitHub Actions records the result for each standalone repository commit. Artifacts include manifest, docs, PE architecture, package content and SHA256 checks.

Automation sends directed messages only to owned fixtures. It does not write the clipboard or exercise successful Explorer actions. Test processes are stopped afterward and pointer restoration is attempted; when this desktop rejects movement the original pointer is unchanged. The user's Winshun and other plugins were not stopped or modified. Development directories whose cleanup was not completed in earlier iterations remain. This iteration keeps regenerable files in ignored `build/tag-msvc/` and packages in `dist/tag-v0.1.0/`.

## 5. Not tested

- User feedback does not itemize physical click/drag isolation, Escape/focus/disable cleanup, copying or successful Explorer actions. Directed automation cannot supply that evidence.
- Physical 100/150/200% mixed DPI, negative monitor coordinates, large text on small screens, administrator/UIAccess/UAC secure desktops, other sessions, protected/hung processes, cross-process embedding, custom transparent hit-testing and screen readers.
- Full MSVC desktop integration, 100-cycle endurance, resource and p50/p95 calibration, every Windows version and code signing.
- A GitHub Release and gallery entry are outside this repository-and-tag task.

For future itemized acceptance, use a locally interactive desktop and click-count fixture. Check that picking does not activate the target, cancellation leaves no surfaces, and monitor hit-testing/scaling is correct. Then verify clipboard contents, Explorer selection, keyboard/screen readers and resource use. Reading ordinary fixtures does not imply that all processes are readable.
