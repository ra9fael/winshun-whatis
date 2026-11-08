# 仓库约定

[English](AGENTS.en.md)

提交标题和正文采用英文 Angular 格式：`type(scope): imperative English summary`，破坏性变更使用 `BREAKING CHANGE:` 正文。

Actions 公开入口只允许新版本 tag 推送与 `workflow_dispatch`，不得添加分支 push、PR 或定时触发。tag 任务必须检查 `github.event.created && !github.event.deleted`。

手动验证接受源码 ref，`publish` 默认为 false；发布要求已有且匹配的版本 tag。保留全部测试与清单、文档、包验证。仅发布任务拥有 `contents: write`，不得覆盖已有 Release 资产。
