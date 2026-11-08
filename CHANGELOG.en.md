# Changelog

[中文](CHANGELOG.md)

## [Unreleased]

## [0.1.2] - 2026-11-09

### Changed

- Publish the executable and window icon fixes from local version 0.1.1.
- Limit release entry points to new version tags and manual runs, retaining CTest, manifest, bilingual documentation and package validation.
- Preserve the existing local tags and use 0.1.2 for the first formal release.

## [0.1.1] - 2026-11-09

### Fixed

- Embed the executable icon and include the ICO and resource script in the package. Assign large and small window icons explicitly, refresh them for DPI changes and release owned handles. Draw the same plugin icon in the custom caption.

### Notes

- Local tag and package only. See docs/icon-validation.en.md for the measured coverage; no GitHub release. Restart the plugin process after upgrading.


## [0.1.0] - 2026-11-08

### UI improvements

- Reorganized the compact result card with a unified caption, program glyph, prominent EXE, distinct path surface, paired actions, and footer details/repick controls.
- Chinese/English font selection, antialiased icons, hover/focus feedback, an empty state without irrelevant fields, and routine ownership notes inside details.
- At 100% scale the default window height shrank from approximately 484 px including its caption to 388 px. Host theme, corners and sizing remain supported.
- Added 52 layout/state checks; 31 integration and 12 real-host regression checks passed.

### Added

- Initial version 0.1.0: native window picking, EXE/path/PID, path copy, host file reveal and window details.
- Temporary input surfaces, picker dragging, Escape restoration, timeout, historical snapshots and access-denied states.
- Host appearance and Chinese/English localization, asynchronous tool bridge, read-only diagnostic CLI and deterministic development packaging.
- Core, real Win32 fixture, fake-host and isolated production Winshun host tests.

### Known limitations

- Initial installation requires confirmation of `ui`, `clipboard:write` and `winshun:open` permissions.
- Windows can prevent foreground activation; follow the prompt to click the card's picker button.
- Secure desktops, other sessions, protected processes and custom transparent-window hit-testing are not guaranteed. A host process is not necessarily its content application.
- The user tested and accepted functionality and UI on November 8, 2026 and authorized the remote repository and version tag. Automation uses directed messages; it does not establish physical click isolation, mixed DPI, clipboard or successful Explorer actions, screen-reader support or performance calibration.
- MSVC Release compilation and CTest pass. The current desktop rejected test cursor movement, so MSVC desktop integration could not be revalidated. The prior GCC build retains its recorded 31 integration, 12 real-host and 52 layout checks.
