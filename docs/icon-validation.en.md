# Icon fix 0.1.1

[中文](icon-validation.md)

## Problem

Embedding an ICO alone can leave native windows using a generic icon.

## Cause

Executable resources do not assign window icons. IDI_ICON1 is a string resource name. Qt windows need an explicit application icon resource.

## Changes

Embed the executable icon and include the ICO and resource script in the package. Assign large and small window icons explicitly, refresh them for DPI changes and release owned handles. Draw the same plugin icon in the custom caption.

## Measured

2026-11-09: built a committed snapshot with MSVC 19.51 / x64 Release; CTest 2/2 passed. Resource and real plugin process checks: 21/21 passed (docs/icon-checks.json), including the named resource, eight sizes, protocol initialization and clean shutdown. The dedicated icon regression also passed 46/46 checks, including DPI and 66 appearance changes; the user previously confirmed the installed icon works.

## Not tested

Physical taskbar/Alt+Tab interaction, every plugin feature, hardware paths, dragging across actual monitors, UAC secure desktop and all Windows versions were not retested. This run creates local tags and packages only; nothing is published. An earlier host probe that actually started whatis exited with 0xC0000005; the original plugin reproduced it too. Standalone icon checks shut down cleanly. That existing host issue was not fixed here, so full release acceptance is not claimed.
