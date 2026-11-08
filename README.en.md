# What Is This · Winshun plugin

[中文](README.md)

Point at a Windows window to inspect its owning process, full EXE path and PID. This native Win32 plugin can copy the path, reveal the file and expand window details.

The refreshed compact card prioritizes the program and path, places Copy/Reveal side by side, and moves details/repicking to the footer. It follows the host's light/dark theme, round/square corners and UI sizing.

**Current version: 0.1.0.** On November 8, 2026, the user tested and accepted the functionality and UI, then authorized the remote repository and tag. Real Winshun host checks pass; coverage limits are documented in the verification notes. Ownership means the process that created the window. A browser page, a command inside a terminal, or content inside a packaged-app host may belong to something else.

## Install and use

Requires Windows x64 and Winshun 0.22.0 or later. This iteration was tested with a development build of Winshun 0.22.0. Earlier hosts, Windows ARM64 and every Windows release were not tested.

1. In Winshun's plugin settings, use the local archive installation entry to select the `.wsplugin` package, confirm its declared permissions and enable it.
2. Enter `whatis`, `这是什么` or `窗口是谁` in the launcher and press Enter.
3. Move the pointer to preview a window and click to lock it. You can also drag the picker button from the result card and release to lock. Escape cancels and restores the previous result.
4. Use **Copy path**, **Reveal file** or **Window details**. Reveal selects the file; it does not run the target EXE.

If Windows does not give the plugin foreground focus, the result card asks you to click **Pick a window**. Picking times out after 30 seconds and cancels on foreground loss or an input desktop change. Closing the card stops scanning; the resident process exits when the host disables it or shuts down.

Paths and window titles are selectable. Access denial preserves the PID and Windows error code while disabling path actions. **Copy details (with path)** includes the title, path and window identifiers. A closed target becomes a historical snapshot; PID reuse does not replace it with a different process.

## Permissions and data

| Permission | Purpose |
|---|---|
| `ui` | Native UI, temporary input surfaces, highlights and other-window lookup |
| `clipboard:write` | Write paths or diagnostics only after a copy action |
| `winshun:open` | Request the host's `show_in_folder` tool |

Queries use `PROCESS_QUERY_LIMITED_INFORMATION`. There is no automatic elevation, process termination, network access or EXE-content reading. Only the latest result is held in memory; ordinary operation writes no path history. Explicit `--probe` output and copied diagnostics may contain personal paths and titles.

## Development

Requires C++17, CMake 3.21+ and a Windows x64 compiler. The plugin has no Qt or WebView dependency. MinGW GCC 13.2 was tested. MSVC 19.51 Release compilation and CTest 2/2 also pass. Desktop integration and user acceptance refer to the GCC build; see the verification notes.

```powershell
cmake -S . -B build -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python tests/integration.py
python tests/visual_states.py
python tests/host_smoke.py --host ../WinShun/build/debug/WinShun.exe --qt D:/tools/Qt/6.12.0/msvc2022_64
python tools/package.py --dev
```

The executable is written to the plugin root. Tests require Python 3.10+ and Pillow. The host test requires a Winshun development build supporting `--probe-runtime --live`. `--qt` is optional for hosts without deployed Qt DLLs; substitute your local Qt kit. Tests control their own windows, temporarily move and restore the pointer, and never write the system clipboard.

L0 validation uses `jsonschema` and the vendored, pinned plugin manifest schema:

```powershell
python -m pip install Pillow jsonschema
python tests/validate.py
```

Development packaging uses an explicit file list, allows uncommitted source, and writes to `dist/development/` with SHA256 and bilingual notes. Release packaging uses Git-tracked files; `--tag` requires a matching version, CHANGELOG section and clean worktree. Packaging does not publish or update the gallery.

Read-only diagnostic: `whatis.exe --probe 0x123456` emits JSON and returns 0 for a readable path, 1 for an unavailable path, or 2 for invalid arguments. It does not open the picker.

## Implementation and verification

[Screenshots](docs/screenshots.en.md) show the accepted light/dark UI. Standalone CI builds with MSVC, runs CTest, validates the manifest/docs/package on main, PRs and version tags, and retains a `whatis-windows-x64` Actions artifact. Tags are pushed after main CI succeeds. CI does not create a GitHub Release or submit the gallery entry.

See [implementation](docs/implementation.en.md), [verification and manual acceptance](docs/validation.en.md), [changes](CHANGELOG.en.md), and [SDK attribution](THIRD_PARTY.en.md).

- L1 / native basics: CTest 2/2 passed.
- L2 / actual Win32 fixtures: 31 checks passed, including real DACL denial, timeouts, late responses, cancellation and shutdown.
- L3 / actual host: 12 checks passed, covering installation, query/action, process inspection, reuse, host tool error roundtrip and shutdown.
- UI revision: 52 state/layout checks passed, including empty/expanded/closed states, both languages and themes, compact/roomy sizing and a synthetic DPI message. The latter does not establish physical mixed-monitor acceptance.
- User acceptance: functionality and the revised UI accepted on November 8, 2026; no device/edge-case checklist was supplied.
- Not automatically covered: physical input isolation, mixed DPI, administrative/UIAccess/secure desktops, screen readers, clipboard and successful Explorer actions, and performance calibration.

Global synthetic input returned success on this desktop, but an independent test window received no clicks. Automation therefore uses directed window messages; these cannot establish physical click isolation. Do not describe this as universal window identification or complete end-to-end acceptance.

MIT licensed; see [LICENSE](LICENSE).
