# 第三方说明

[English](THIRD_PARTY.en.md)

`winshun.h` 来自 LingCore/WinShun 的 `src/plugins-sdk/winshun.h`，基准提交 `1769c368`，MIT 许可，Copyright (c) 2026 LingCore。许可证全文保留于 [LICENSE](LICENSE)。

本地改动：新增 `toolAsync` / `cancelTool`，使用互斥锁保护最多 8 个待回复回调，以 `whatis-tool-N` 字符串 ID 区分请求，在 serve 线程派发回复。插件 UI 将回调再投递到自身线程，并实现超时和代次检查。同步 `tool()` 接口及 JSON-RPC 线协议未改变。此扩展仅在插件的 SDK 副本中，未修改 Win顺主仓库 SDK。

运行时链接 Windows 系统 DLL。GCC 构建静态链接 libgcc / libstdc++，适用 GCC Runtime Library Exception；未分发 Qt 或 WebView 运行时。Pillow 和 jsonschema 仅用于开发验证，不进入运行时包。

`tests/schema/plugin.schema.json` 来自 LingCore/WinShun-plugins，固定提交 `9b13660df5b4a15a6149841ffba772b5e4b47fd3`，未修改，MIT 许可，Copyright (c) 2026 LingCore；许可证见 [LICENSE](LICENSE)。
