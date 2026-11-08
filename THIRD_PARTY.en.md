# Third-party attribution

[中文](THIRD_PARTY.md)

`winshun.h` comes from `src/plugins-sdk/winshun.h` in LingCore/WinShun at base commit `1769c368`, under the MIT license, Copyright (c) 2026 LingCore. The full license is retained in [LICENSE](LICENSE).

Local changes add `toolAsync` / `cancelTool`, a mutex-protected map of at most eight pending callbacks, `whatis-tool-N` string request IDs, and response dispatch on the serve thread. The plugin posts callbacks to its UI thread and checks timeouts and generations. Synchronous `tool()` and the JSON-RPC wire protocol are unchanged. This extension exists only in the plugin's SDK copy; the host SDK repository was not modified.

Runtime imports are Windows system DLLs. The GCC build statically links libgcc / libstdc++ under the GCC Runtime Library Exception. No Qt or WebView runtime is distributed. Pillow and jsonschema are validation dependencies only and are excluded from the runtime package.

`tests/schema/plugin.schema.json` is an unmodified copy from LingCore/WinShun-plugins at pinned commit `9b13660df5b4a15a6149841ffba772b5e4b47fd3`, MIT licensed, Copyright (c) 2026 LingCore; see [LICENSE](LICENSE).
