# [0612] 新增独立的 Quiver 交换图插件与会话支持

## 1 相关文档
- [0612.md](0612.md) - TikZ 插件基础开发与重构任务文档

## 2 任务相关的代码文件
- `TeXmacs/plugins/quiver/progs/init-quiver.scm` — 插件初始化与会话注册
- `TeXmacs/plugins/quiver/progs/data/quiver.scm` — 注册 `quiver` 格式与转换器
- `TeXmacs/plugins/quiver/progs/code/quiver-mode.scm` — Quiver 编辑模式检测
- `TeXmacs/plugins/quiver/progs/code/quiver-edit.scm` — Quiver 编辑器特性、快捷键、括号配对
- `TeXmacs/plugins/quiver/progs/code/quiver-lang.scm` — Quiver 词法着色与语法高亮
- `TeXmacs/plugins/quiver/goldfish/tm-quiver.scm` — Quiver 核心编译、包注入逻辑
- `TeXmacs/plugins/quiver/tests/tm-quiver-test.scm` — Quiver 单元测试用例

## 3 如何测试

### 3.1 单元测试（确定性测试）
1. 构建 Goldfish Scheme 解释器：
   ```bash
   xmake b goldfish
   ```
2. 运行 Quiver 插件单元测试：
   ```bash
   TeXmacs/plugins/goldfish/bin/goldfish load TeXmacs/plugins/quiver/tests/tm-quiver-test.scm
   ```

### 3.2 会话与文档验证（非确定性测试）
1. 构建并运行 Mogan STEM：
   ```bash
   xmake b stem
   xmake r stem
   ```
2. 手动验证：通过菜单 `Insert -> Session -> Quiver` 插入 Quiver 交换图会话：
   - 验证可以正常输入交换图代码（如 `A \arrow[r] & B`）并自适应生成正确的 PDF 预览。
   - 验证语法高亮是否正常染色（如 `arrow`, `begin`, `end`, `tikzcd` 等关键词着色）。
   - 验证括号、括号配对和自动对齐功能是否正常。

## 4 What
实现一个高内聚、低耦合、完全独立的 `quiver` 交换图插件：
1. 采用与 `tikz` 插件一致的现代 Goldfish Scheme 架构进行开发。
2. 在 `Insert -> Session` 菜单中新增专属的 `Quiver` 会话入口。
3. 自动注入完整的、自包含的 quiver 支持宏定义及 TikZ 依赖库（`tikz-cd`, `amssymb`, `calc`, `spath3` 等），使用户在无需安装 `quiver.sty` 本地库的机器上亦能渲染包含高级 quiver 属性（如 curve，between 比例缩短等）的精美图表。
4. 保证在代码最外层不是 `tikzcd` 时能自动完成环境包裹，是 `tikzcd` 时能绕过 `tikzpicture` 环境，以实现无痛渲染。

## 5 Why
1. **职责分离（Separation of Concerns）**：Quiver 虽然生成的是 TikZ/tikz-cd 代码，但它代表了一个独特的“交换图编辑/呈现”领域，拥有自己特定的宏包依赖、样式集及最顶层环境。将其与基础 TikZ 绘图混在一起，不仅会让 TikZ 的代码膨胀，还会增加对无关图表的干扰风险。
2. **完美的用户体验**：作为一个单独的插件注册，让用户能直接插入 `Quiver` 会话。
3. **独立测试与演进**：有了单独的测试集和代码文件，后期针对 Quiver 的快捷键优化、宏包升级都更加安全和便利。

## 6 How
1. **测试驱动开发 (TDD)**：先实现 `tm-quiver-test.scm` 以验证 `wrap-quiver-code` 的自动包注入和环境自适应逻辑，确认测试失败后再在 `tm-quiver.scm` 中写出最终实现，保证其 100% 运行通过。
2. **注册独立会话**：在 `init-quiver.scm` 中，通过 `(plugin-configure quiver ... (:session "Quiver"))` 完成高层会话绑定与命令启动。
3. **语言解析与语法高亮**：创建 `quiver-lang.scm` 词法文件，定义对 `arrow`, `tikzcd`, `begin`, `end` 等特有结构的着色支持。
