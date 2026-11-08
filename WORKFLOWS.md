# 构建与发布

[English](WORKFLOWS.en.md)

推送新的 `vX.Y.Z` 标签会构建、测试、打包并发布；修改或删除已有标签不会构建。分支 push 和 PR 不触发 Actions。

手动运行 `release.yml`，设置源码 `ref`，保持 `publish=false`，即可验证并下载 artifact。默认不会发布。发布时必须将 `ref` 设为已有版本标签并启用 `publish=true`，版本、变更日志和分支归属均须匹配。

流程只构建一次，执行原有 CTest、清单 schema、双语文档、PE、包内容与 SHA-256 校验后，发布同一份产物。发布前再次检查标签对应的提交；已有 Release 会明确报错，绝不覆盖资产。

提交采用英文 Angular 格式；只校验当前待发布提交，不改写无关历史。
