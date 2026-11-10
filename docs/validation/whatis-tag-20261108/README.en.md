# What Is This first repository and tag verification

[中文](README.md)

## 1. Problem

On November 8, 2026 the user reported successful testing and authorized a remote GitHub repository and new tag. The plugin had no commits or remote; its manifest version was `0.1.0`.

## 2. Cause

The initial publication needs standalone CI, a matching changelog, verifiable packages and precise acceptance claims. During MSVC desktop revalidation, `SetCursorPos(248,180)` returned 0 without moving the pointer, so directed picking inspected another window at the original pointer position. The previously passing GCC binary reproduced the same desktop restriction. That unrelated window was not operated. Tests now check the precondition and report diagnostics; this desktop revalidation is not counted as passing.

## 3. Changes

- Created public repository `ra9fael/winshun-whatis`, default branch `main`.
- Commit `5457727a3cce26fe551363d456f5300f94832975` includes the plugin, bilingual docs, selected screenshots, acceptance summary, pinned schema, CI and version packaging.
- The MSVC fixture explicitly links `advapi32`. Packaging extracts only the current changelog section with correct bilingual note links.
- Created and pushed annotated tag `v0.1.0` only after main CI passed. The tag object is `fea5303e425e16e64f7255438221caf64b2e2b1d`.

## 4. Measured

- Local MSVC 19.51 Release compilation and CTest 2/2 passed. L0 schema, 12 bilingual documents, PE x64 and package/hash validation passed; see `static.json`.
- Earlier GCC results remain: 31/31 desktop integration, 12/12 real-host and 52/52 layout/state checks plus user acceptance; see `acceptance.json`. General user feedback is not expanded into itemized edge-case acceptance.
- Main CI `42477691042` and tag CI `42477801178` succeeded; see `main-ci.json` and `tag-ci.json`. Both built with Windows MSVC, ran CTest, packaged, validated manifest/docs/package and uploaded artifacts.
- Remote main and dereferenced tag match the commit above; see `refs.txt`. The repository is PUBLIC, default branch main, and the plugin worktree is clean.
- Downloaded the tag's `whatis-windows-x64` artifact and verified archive CRC, manifest version, EXE/package hashes, checksum file and every packaged source against the tagged commit; all passed. See `artifacts.json`, `remote-package.json` and `remote-verification.json`.
- Remote package SHA256: `e43f7fbff186ac52ca5d148dd06d83437ef51d6aa11899134c94073b11c3068a`. The local MSVC 19.51 package is separately recorded in `local-package.json`; hashes from different toolchains are not interchangeable.
- Test-owned processes are closed. The user's Winshun and other plugins were not stopped. Current build/diagnostic evidence remains in ignored `ra9fael.whatis/build/tag-msvc/` and the local version package in `dist/tag-v0.1.0/`. Automatic approval rejected removal of this iteration's `dist/tag-v0.1.0-preview/` with `blocked by policy` and no specific reason. The directory remains; no bypass was attempted. Older leftovers and unrelated repository edits were untouched.

## 5. Not tested

Full MSVC desktop revalidation could not complete in this environment. Physical click isolation, real mixed DPI, other sessions/secure desktops/UIAccess/protected processes, screen readers, automated clipboard/successful Explorer paths and long-term resource/performance calibration were not added. Actions emitted a Node 20 deprecation notice while forcing those actions onto Node 24; every step succeeded. No GitHub Release or gallery entry was created; this delivery covers the repository and tag.
