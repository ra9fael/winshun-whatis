# Verification and manual acceptance

[涓枃](README.md)

Date: November 8, 2026. This development preview is not ready for publication. Curated machine-readable evidence is kept in the workspace host repository under `whatis-native-20261108/`.

## 1. Problem

Verify actual Windows window-to-EXE inspection and the real Winshun protocol, permissions, lifecycle and native interactions. HTML checks cannot replace native acceptance. Environment: Windows x64, GCC 13.2, Winshun 0.22.0 development build, host HEAD `1769c368`.

## 2. Cause

Processes may deny reads, asynchronous results may arrive late, and tool calls must not block protocol handling. Selection surfaces affect input and hit-testing. On this desktop, synthetic global input APIs returned success while an independent test button received no clicks. Their return values therefore cannot establish physical input success.

## 3. Changes

Added the independent `ra9fael.whatis` plugin, limited-access inspection, generation state machine, temporary input surfaces, native card, asynchronous host tools, tests and development packaging. Host code is unchanged. See [implementation](../../../../ra9fael.whatis/docs/implementation.en.md) for architecture and prototype deviations.

## 4. Measured

| Layer / design area | Method | Result |
|---|---|---|
| L0 / manifest, docs, package | `tests/validate.py`, duplicate deterministic generation, PE imports | Schema, bilingual links, x64, archive contents/hash passed |
| L1 / state | CTest `whatis_core` | Generations, cancellation, ordering, identity mismatch and host labels passed |
| Native basics / platform | CTest `whatis_native` | Self path, creation time, Unicode and closed-window detection passed |
| L2 + Win32 fixtures / UI, protocol | `tests/integration.py`, real fixture windows, fake stdio host, directed messages | 31 passed: actual process DACL denial, late replies, timeout, closure and handle counts across 12 cancellations |
| L3 / host integration | `tests/host_smoke.py`, isolated `--probe-runtime --live` | 12 passed: install, query, action, normal-sandbox read, repeat action, real missing-file tool error and shutdown |

PrintWindow screenshots of test windows were visually reviewed in light, English dark and access-denied states. Physical input is outside these passing claims. Tests did not write the clipboard or open Explorer; the pointer was restored afterward.

Evidence: `static.json` covers L0; `ctest.txt` contains 22 core assertions and native basics; `integration.json` contains 31 integration checks; `host-smoke.json` contains 12 real-host checks; `package.json` records archive contents/hashes; `imports.txt` lists PE imports. Handle counts remained 168 before and after 12 cancellations. Screenshots: `after-light.png`, `after-dark-en.png`, `after-access-denied.png`, `host-query.png`, `after-host-result.png`.

Verified EXE SHA256: `803d6fe9f7ceaf3247d5b862c2b62a0d684bdb75c2c35712badb1a12dbe1ede9`. Development package SHA256: `a01e51bafd91694a704079f8d17efbe9fa3b7c55d0821718b06919c9160c11a2`. The host test installed a directory; the archive passed ZIP, manifest, byte and hash checks. Importing it through the plugin settings UI was not exercised.

The host preview attempts profile removal before App destruction, leaving some occupied files. The driver cleans the exact observed temporary profile after host exit. Initial startup also failed without deployed Qt DLLs; supplying `--qt` resolved it. Both are host test-environment issues. The user's running Winshun was not changed. Sources remain uncommitted and unpushed.

In one follow-up run, deployment inspection outlasted the initial foreground guard. The driver now restarts picking from its owned card after inspection; the final 12 checks passed. This remains directed-message coverage. `host-preview.txt` preserves the preview cleanup watcher warning. The successful run's profile was cleaned after exit; the host warning is not classified as a plugin failure.

Final cleanup was rejected by automatic tool review with only `blocked by policy`. The earlier failed-run profile `C:\Users\renfei\AppData\Local\Temp\WinShun-preview-8a9ab7c7`, plugin folders `build/host-fixtures`, `build/host-target`, `build/host-smoke`, `build/validation-deps` and `tests/__pycache__` therefore remain. These are task-generated artifacts, not user configuration. The rejection was not bypassed. Source, development package and curated evidence are intact.

## 5. Not tested

- **L4 publication blockers:** physical click/drag isolation from target buttons, keyboard Escape, foreground transitions and disable cleanup; actual clipboard and successful Explorer actions.
- **L4 publication blockers:** 100/150/200% mixed DPI, negative monitor coordinates, large text on small screens, administrative/UIAccess windows, UAC secure desktop, other sessions, protected/hung processes, cross-process embedding, custom transparent hit-testing and screen readers.
- **L5 outstanding:** 100-cycle endurance, resource and p50/p95 calibration, different Windows releases, MSVC compilation, signing, repository CI, public Release and gallery submission.
- Reading ordinary fixtures does not prove all processes are readable. Twelve handle-count cycles do not prove long-term stability. System hit-testing does not prove physical click isolation.

### Manual steps before publication

Install the development package on a locally operable test desktop. Prepare a click-count fixture, ordinary applications and an administrative application. Pick by click and drag, verify ownership and ensure the target click counter does not increase. Cancel with Escape, change focus, lock/unlock and disable the plugin; confirm no input surface remains. Check PID and error code for unreadable processes.

Repeat on 100/150/200% mixed-DPI monitors, including negative coordinates; verify borders, hit-testing, hints and all buttons remain usable. Manually copy a nonsensitive path and reveal a fixture EXE, confirming Explorer only selects the file. Record screen-reader and keyboard behavior, memory/handles across 100 cycles and latency. Resolve failures and repeat acceptance before publication.

