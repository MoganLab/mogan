# Python 可复现科研工作流 (Research Workflow)

## 为什么需要此文档
现有的 `python.zh.tmu` 侧重于单项功能的详细 API 与用法介绍，类似于“功能词典”。本工作流文档旨在提供一个“端到端”的闭环示例，帮助用户直观理解如何在实际科研场景中串联 Conda 环境、数据分析与结果渲染。

## 它与现有 `python.zh.tmu` 的关系
- **定位互补**：`python.zh.tmu` 负责广度，涵盖所有支持的库与操作；`workflow.zh.tmu` 负责深度与链路，演示实际应用逻辑。
- **引用关系**：在 `python.zh.tmu` 末尾添加了指向该工作流文档的入口，实现从功能学习到实操应用的引导。

## 覆盖的能力边界
- **Conda 会话集成**：验证 `Plugins -> Sessions -> Python` 菜单下的动态环境加载。
- **端到端执行 (Executable Workflow)**：验证在同一个 Session 上下文中，数据从 Pandas 传递到 Matplotlib 的连续执行能力。
- **综合渲染支持**：验证表格、矢量图与数学公式的自动转换与渲染。

## 维护者如何手动验证
1. **准备环境**：在终端运行 `conda create -n mogan_test python=3.10 pandas matplotlib sympy`。
2. **执行验证**：
   - 启动 Mogan STEM，打开 `TeXmacs/plugins/python/doc/workflow.zh.tmu`。
   - 确保 Session 切换为 `conda_mogan_test`。
   - 依次执行 Pandas、Matplotlib 和 SymPy 代码块。
3. **验收标准**：
   - 确认 Pandas DataFrame 自动转为规整表格。
   - 确认 Matplotlib 绘图输出为不失真的 PDF 矢量图。
   - 确认 SymPy 对象自动渲染为数学公式。

## 说明
为保持首个贡献的最小化与稳定性，本次 PR 未引入独立的自动化测试文件，验证逻辑完全集成在用户可见的示例文档中，方便维护者与用户共同监督。
