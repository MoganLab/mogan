# 开发规范

## 分支命名规则

分支格式：`username/200_27/xxx`

- `username`: 开发者用户名
- `200_27`: 项目标识符
- `xxx`: 功能描述或任务编号

例如：
- `da/200_27/xmake_debug`
- `da/200_27/fix_pdf_rendering`

## 代码推送规则

1. 如果 remote 是 GitHub，使用 `gh` 命令推送代码并创建 PR
2. 如果 remote 是 Gitee，直接使用 `git push` 推送代码
3. 推送前确保代码已通过本地测试
4. 保持提交信息清晰、简洁

## 工作流程

1. 基于主分支创建新分支
2. 按规范命名分支
3. 开发完成后直接 `git push` 推送
4. 不需要使用 GitHub CLI 工具