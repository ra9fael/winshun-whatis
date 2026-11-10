# What Is This 首次仓库与标签验证

[English](README.en.md)

## 1. 问题是什么

用户于 2026-11-08 表示已经测试合适，授权建立远端 GitHub 仓库并打新标签。插件此前没有提交或远端；manifest 为 `0.1.0`。

## 2. 为什么

首次发布需要独立 CI、与标签一致的变更记录、可复核包和准确的验收范围。新增 MSVC 桌面复验中 `SetCursorPos(248,180)` 返回 0，指针未移动，导致定向选择读到了指针原位置的其他窗口；原已通过的 GCC 二进制同样复现，确认为当前测试桌面的输入限制。未操作那个非测试窗口。测试增加前置检查和失败诊断；没有将本轮桌面复验记为通过。

## 3. 改了什么

- 创建公开仓库 `ra9fael/winshun-whatis`，默认分支 `main`。
- 提交 `5457727a3cce26fe551363d456f5300f94832975` 包含插件、双语说明、精选截图、验收摘要、固定 schema、CI 和版本打包。
- MSVC 测试辅助程序显式链接 `advapi32`；打包只提取当前版本的 CHANGELOG 并修复双语互链。
- main CI 通过后才创建并推送附注标签 `v0.1.0`。标签对象为 `fea5303e425e16e64f7255438221caf64b2e2b1d`。

## 4. 实测

- 本机 MSVC 19.51 Release 构建与 CTest 2/2 通过；L0 清单 schema、12 份双语文档、PE x64 和包内容/哈希校验通过，见 `static.json`。
- 此前 GCC 桌面测试 31/31、真实宿主 12/12、布局状态 52/52 及用户验收保留，见 `acceptance.json`；用户反馈不被扩展为每个边界的验收记录。
- main CI `42477691042`、tag CI `42477801178` 均 success，见 `main-ci.json` 与 `tag-ci.json`。均完成 Windows MSVC 构建、CTest、打包、清单/文档/包验证和上传产物。
- 远端 main 与标签解引用均为上述提交，见 `refs.txt`。仓库 PUBLIC，main 为默认分支，插件工作树干净。
- 下载标签 CI 的 `whatis-windows-x64` 产物，复核归档 CRC、manifest 版本、EXE/包哈希、校验文件，并逐文件与标签提交比较所有打包源码，全部通过，见 `artifacts.json`、`remote-package.json`、`remote-verification.json`。
- 远端包 SHA256：`e43f7fbff186ac52ca5d148dd06d83437ef51d6aa11899134c94073b11c3068a`。本机 MSVC 19.51 包与远端工具链产物分别记录，不能互换哈希；本机信息见 `local-package.json`。
- 本轮测试进程已关闭；未结束用户的 Win顺或其他插件。本轮构建和诊断证据保存在忽略的 `ra9fael.whatis/build/tag-msvc/`，本地版本包保存在 `dist/tag-v0.1.0/`。自动审批以 `blocked by policy` 拒绝删除本轮 `dist/tag-v0.1.0-preview/`，未提供具体原因，目录保留且未尝试绕过。旧轮次残留未改动，无关仓库修改未提交。

## 5. 没测的

本轮 MSVC 完整桌面交互复验受环境限制未完成。物理点击隔离、真实混合 DPI、多会话/安全桌面/UIAccess/受保护进程、屏幕阅读器、自动化剪贴板与 Explorer 成功路径、长期资源/性能标定未补充验证。Actions 的 Node 20 action 在 runner 上被强制使用 Node 24，出现弃用提示但全部步骤成功。未创建 GitHub Release 或提交插件库条目；交付范围为仓库和标签。
