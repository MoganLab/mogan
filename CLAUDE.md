# 开发规范

## C++ 代码规范

1. **不使用标准 C++ 库**：Mogan 使用自研的 C++ 基础设施（如 lolly/moebius 库），内部类型（`string`、`list`、`array`、`tree`、`path` 等）均有自定义实现，与 `std::` 不兼容。
2. **输出流使用项目内置 `cout`**：调试输出应使用全局 `cout`（类型为 `tm_ostream`），而非 `std::cout`；换行使用 `LF` 宏或 `"\n"`，不要使用 `std::endl`。
3. **容器不支持现代 C++ 特性**：自定义容器（如 `rectangles`、`list`）不支持范围 for 循环（range-based for），需使用传统的迭代器或 `is_nil()`/`next` 遍历。
4. **类型转换使用项目函数**：自定义类型（如 `path`）没有标准 `operator<<` 重载，输出前需先用 `as_string()` 转换。

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