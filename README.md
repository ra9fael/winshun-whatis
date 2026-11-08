# 这是什么 · Win顺插件

[English](README.en.md)

指向一个 Windows 窗口，查看创建它的进程、EXE 完整路径和 PID。插件使用原生 Win32 界面，可复制路径、在文件夹中定位文件、展开窗口详情。

界面已更新为紧凑结果卡：程序名与路径优先显示，复制和定位并列，底部展开详情或重新选取；支持深浅主题、圆角/直角和宿主字号。

**当前版本：0.1.0。** 2026-11-08，用户实测后确认功能与界面合适，授权建立远端仓库并打标签。已通过真实 Win顺宿主测试；未逐项确认的边界见验证文档。这里的“窗口归属”指创建窗口的进程：浏览器页面、终端中的命令、包应用宿主背后的内容应用不一定是同一个程序。

## 安装与使用

需要 Windows x64、Win顺 0.22.0 或更新版本。本次实际测试使用 Win顺 0.22.0 的开发构建；未测试更早版本、Windows ARM64 或每个 Windows 系统版本。

1. 在 Win顺设置的插件页，使用从本地归档安装的入口选择 `.wsplugin` 包，确认它声明的权限并启用。
2. 在启动器输入 `whatis`、`这是什么` 或 `窗口是谁`，按 Enter。
3. 移动鼠标预览窗口，单击锁定；也可以从结果卡的准星按钮按住拖动，松开锁定。Esc 取消后保留上次结果。
4. 使用“复制路径”“定位文件”或“展开窗口详情”。定位只选中文件，不启动目标 EXE。

如果 Windows 没有把前台焦点交给插件，会显示结果卡，提示点击“选取窗口”继续。选取会在 30 秒后超时；失去前台或切换输入桌面时取消。关闭结果卡后停止扫描，常驻进程随宿主停用或退出。

路径和窗口标题可直接选中。进程拒绝访问时，保留 PID 与 Windows 错误码，禁用复制路径及定位；“复制诊断（含路径）”会包含窗口标题、路径和窗口标识。目标关闭后显示历史快照，不将旧结果替换成复用 PID 的新进程。

## 权限与数据

| 权限 | 用途 |
|---|---|
| `ui` | 显示原生窗口、选取输入层和高亮，查找其他窗口 |
| `clipboard:write` | 仅在点击复制时写入路径或诊断文本 |
| `winshun:open` | 调用宿主的 `show_in_folder` 定位文件 |

查询使用 `PROCESS_QUERY_LIMITED_INFORMATION`。不自动提权、不结束进程、不联网，不读取 EXE 内容。最近一次结果只保存在内存中；正常运行不写路径历史。显式 `--probe` 诊断输出和复制内容可能包含个人路径及标题。

## 开发

使用 C++17、CMake 3.21+ 和 Windows x64 编译器；插件不需要 Qt 或 WebView。已实测 MinGW GCC 13.2；MSVC 19.51 Release 编译及 CTest 2/2 通过。桌面集成与用户验收对应 GCC 构建，详见验证文档。

```powershell
cmake -S . -B build -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python tests/integration.py
python tests/visual_states.py
python tests/host_smoke.py --host ../WinShun/build/debug/WinShun.exe --qt D:/tools/Qt/6.12.0/msvc2022_64
python tools/package.py --dev
```

`whatis.exe` 输出到插件根目录。测试需要 Python 3.10+、Pillow；宿主测试要求支持 `--probe-runtime --live` 的 Win顺开发构建。`--qt` 是宿主尚未部署 Qt DLL 时的可选参数，应替换为本机 Qt 路径。测试仅控制本次创建的窗口，暂时移动鼠标后还原；不会写系统剪贴板。

L0 校验使用 `jsonschema` 和仓库内固定版本的插件清单 schema：

```powershell
python -m pip install Pillow jsonschema
python tests/validate.py
```

开发包使用明确文件清单，允许尚未提交的源码，输出至 `dist/development/`，附 SHA256 和双语说明。正式打包使用 Git 跟踪文件；`--tag` 要求版本、CHANGELOG 和干净工作树匹配。打包不会发布或更新插件索引。

只读诊断：`whatis.exe --probe 0x123456` 输出 JSON，读取成功返回 0，路径不可读返回 1，参数错误返回 2。它不显示选取界面。

## 实现与验证状态

[界面截图](docs/screenshots.md) 展示已验收的深浅主题。独立仓库 CI 对 main、PR 和版本标签执行 MSVC 构建、CTest、清单/文档/包校验，并保留 `whatis-windows-x64` Actions 产物。标签在 main CI 通过后推送；CI 不自动创建 GitHub Release 或提交插件索引。

详见 [实现说明](docs/implementation.md)、[验证及人工验收](docs/validation.md)、[变更记录](CHANGELOG.md)。SDK 来源和本地扩展见 [第三方说明](THIRD_PARTY.md)。

- L1 / 原生基础：CTest 2/2 通过。
- L2 / 本机 Win32 测试窗口：31 项通过，包括真实 DACL 拒绝访问、超时、迟到响应、取消与退出清理。
- L3 / 真实宿主：12 项通过，包括实际安装、查询与动作、进程读取、复用、宿主工具错误往返及退出。
- 界面更新：52 项状态/布局检查通过，包括空态、展开、关闭、中英文、深浅色、紧凑/宽松和模拟 DPI 消息；后者不代表真实混合屏幕验收。
- 用户验收：2026-11-08 确认功能及改版界面可用；未提供逐项设备/边界测试清单。
- 未自动覆盖：真实输入隔离、混合 DPI、管理员/UIAccess/安全桌面、屏幕阅读器、剪贴板及 Explorer 成功路径、性能标定。

本机合成全局输入虽返回成功，独立测试窗口没有收到点击，所以自动化使用定向窗口消息；它不能证明真实点击不透传。不得据此宣称识别“任意窗口”或完整端到端测试通过。

MIT 许可，见 [LICENSE](LICENSE)。
