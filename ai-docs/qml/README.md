# QML 对话框

`src/Plugins/Qt/qml/` 的弹窗体系。新增对话框先读这份，避免踩已知的坑。

## 规矩：对话框必须模态；按需选两种模态引擎

**所有 QML 对话框必须模态**（独占输入：防切窗口/动光标/重复打开），不用纯非模态 `show`。
纯非模态会丢失这些语义，要靠 hack 补回，不可取。

模态有两种实现，按「是否需要 live 重绘文档」选：

| 引擎 | 实现 | 阻塞 | 主窗口 live 重绘 | 适用 |
|------|------|------|-----------------|------|
| `run_qml_dialog` | `exec()` | 是（嵌套事件循环） | **不重绘** | 一次性提交（ConfirmClose/FormDialog） |
| `run_modal_qml_dialog` | `setModal(true)+show()` | 否 | **重绘** ✅ | live 写回文档（FontSelector） |

**关键坑**：`exec()` 的嵌套事件循环**不重绘被遮的主窗口**——selector-set 改了文档树但
画面不更新，要点 OK 关窗才看到。需要 live 实时看到效果的对话框必须用 `setModal+show`
（非阻塞模态）：它仍由 Qt 按 modality 拦截其他窗口输入（独占），但不嵌套事件循环，主
窗口 paint 照常。字体选择器曾用 exec 导致 live 不重绘，改 `setModal+show` 后兼得独占
与重绘。

生命期：`run_modal_qml_dialog` 堆分配 QDialog + `WA_DeleteOnClose`，bridge 不挂 parent，
靠 `connect(host, destroyed, bridge, deleteLater)` 自清；OK/Cancel 调 `close()`（触发
WA_DeleteOnClose → delete，注意 `done()` 不走 closeEvent、不触发 delete）。样板见
`cpp_font_selector_dialog`。

## Cancel/重置撤销（live 写回对话框）

live 写回的对话框（FontSelector）需要 Cancel/重置撤销已应用的改动。**不要用 mark-cancel**
（mogan 的 undo mark 事务）——它有两个缺陷：

- 对 init 块改动无效（文档字体走 init-multi 改 init，不进 undo 历史，mark-cancel 回滚不了）
- 对 buffer 改动丢选区（mark-cancel 的 apply 触发 post_notify→selection_cancel）

正解是**快照写回**：对话框打开瞬间（register-specs，live 改动前）快照文档字体
（`initial-snapshot`）；Cancel/重置共用 `font-selector-revert-to-snapshot`——快照填
selector-table，再 `selector-get-changes` + **一次** setter 写回。一次 setter 避免多次
make-multi-with 嵌套吞选区；两条 setter 路径（init-multi / make-multi-with）都适用。

## 板块

`src/Plugins/Qt/qml/` 分两层目录：

- **`atoms/`** — 原子板块（拼装用，勿自造外壳/配色），各有 `atoms/qmldir` 登记。
  成品弹窗 `import "atoms"` 取用（含 `Theme` 单例）。
- **`qml/`**（本目录）— 成品弹窗，`qml/qmldir` 登记。

**原子**（`atoms/`，拼装用，勿自造外壳/配色）：
- `DialogShell` — 外壳：圆角、无边框拖动、Esc、`content` 正文槽、共享下拉浮层
- `DialogButtons` — 按钮行，只发 `clicked(index)`，语义由调用方映射
- `MiniButton` — 紧凑小按钮（正文内辅助按钮组，如行间距预设），只发 `clicked()`
- `EnumCombo` — 下拉 combo 行（浮层由 DialogShell 管），支持 optionsTr 翻译分离
- `EnumComboList` — 可滚动的 EnumCombo 竖列（Filter/Advanced 选项卡内容）
- `SelectableList` — 常驻单选列表（family/style/size 三栏用），自带 `refreshTick`
  驱动 currentValue 重算
- `PreviewPane` — 预览图区（显示 bridge 光栅化的 PNG data URL）
- `TabBar` — 胶囊选项卡行
- `TabPanel` — 带选项卡的容器面板（TabBar + content 槽）
- `Theme` — 主题单例（scaleFactor / 暗色 / 配色 / 结构尺寸常量 `rowH`·`btnH`）

**成品**：
| 对话框 | 模态引擎 | 模型 |
|--------|---------|------|
| `ConfirmClose` | `run_qml_dialog`（exec） | 点按钮返回结果 |
| `FormDialog` | `run_qml_dialog`（exec） | 本地暂存 `values`，OK 一次性 submit |
| `FontSelector` | `run_modal_qml_dialog`（setModal+show） | live 写回文档，OK 落定 / Cancel+重置快照撤销 |
| `ParagraphFormat` | `run_modal_qml_dialog`（setModal+show） | live 写回（段落 with / 文档 initial），按 scope 撤销 |

## 编码规矩

- **原子内部 id 不用 `root`**：用组件语义缩写（`shell`/`row`/`comboRow`/`selList`/`bar`/
  `panel`/`pane`/`btn` 等）。这样调用方 delegate 里的 `root.xxx` 指向调用方根，不被原子
  内部同名 id 遮蔽。成品弹窗根 id 统一用 `root`。`DialogShell` 的 `objectName:"DialogShell"`
  是运行时查找标记（EnumCombo 沿 parent 链找它），勿改。
- **尺寸/字号走 `Theme` 命名常量**：结构尺寸 `Theme.rowH`/`btnH`/`itemH`，圆角
  `Theme.radius`/`radiusLg`，字号阶梯 `Theme.fontBody`/`fontBtn`/`fontTab`/`fontMini`/
  `fontTiny`。原子组件内的这些固定值都应引用常量；胶囊圆角用 `radius: height / 2`
  （= 高的一半），不写字面量。成品弹窗内的拼接布局数字（列宽、间距、行高按字段算等）
  可保留字面量。
- **cancel 走语义化入口**：`QmlDialogBridge.cancel()`（= `done(Rejected)`），勿散落
  `choose(-1)` 魔法值。`choose(n>=0)` 仍用于「选第 n 个按钮」（ConfirmClose）。
- **绑定到无参 bridge 函数的属性不会自动重算**：bridge 内部状态变化 QML 感知不到，需靠
  外部计数器注入依赖。`SelectableList.refreshTick` 已封装此模式——调用方 currentValue 绑定
  读 `<listId>.refreshTick`，refresh 时 `<listId>.refreshTick++` 即可，勿手写假读样板。
- **`Rectangle` 的 `border` 与 `clip:true` 互斥**：Qt 的 border 以边缘为中心绘制（半内半外），
  同一 Rectangle 既 `border` 又 `clip:true` 会把外侧半像素 border 自身裁掉、几乎不可见
  （叠加 `radius` 的抗锯齿偏移更甚）。要带边框的裁剪容器，拆**双层 Rectangle**：外层只画
  border（不 clip），内层 `clip:true` 容纳子项。下拉浮层（DialogShell overlay）即此结构。
- **跨 parent 链查找的 property 不能喂给 binding**：如 EnumCombo 的 `dialogShell`（沿
  parent 链按 objectName 查找）是命令式 property，parent 变化不触发其重算，binding 会在
  创建瞬间读到 null 永久卡住。依赖它的几何（comboX/Y/W）须在展开时用 `updateGeometry()`
  拍快照，不能改成 binding 实时算。

## 模态引擎（QTMQmlDialog.cpp）

两个引擎都收敛了 QDialog 拼装 + setSource + 加载检查 + 定尺寸流程，差异只在 exec vs
setModal+show（见上表）。新增模态对话框写 context 注入回调 + 解读返回值即可，不重抄
宿主拼装。

## 开发工作流（新增弹窗/原子）

新增一个成品弹窗 `Foo.qml`：

1. **设计稿先行**：在 `ai-docs/qml/qml-dialog.html` 加面板，定布局/取值/交互。
2. **QML 正文**：`src/Plugins/Qt/qml/Foo.qml`，根用 `DialogShell`，`import "atoms"` 取
   原子 + `Theme`。根 id 用 `root`；正文引用根属性走 id（content 被 reparent，parent 链
   不可靠）。尺寸/字号尽量复用 `Theme` 常量；拼接布局数字（列宽/间距）可留字面量。
3. **登记**：`qml/qmldir` 加 `Foo 1.0 Foo.qml`；`moganqml.qrc` 加 `<file>qml/Foo.qml</file>`
   （无 glob，漏登则运行期找不到）。新增原子同理放 `atoms/` 并登 `atoms/qmldir`。
4. **bridge + glue**：若需 live 写回，仿 `FontSelectorBridge` 写 C++ bridge（注入 context
   property）；在 `QTMQmlDialog.cpp` 加 glue 入口（选 `run_qml_dialog` exec 或
   `run_modal_qml_dialog` setModal+show，按是否需 live 重绘文档决定）。
5. **加载测试**：在 `tests/Plugins/Qt/qml_load_test.cpp` 加 `test_foo_loads()`，注入对应
   bridge 桩（确认型用 `StubBridge`，live 型用 `StubLiveBridge`），断言 `status()==Ready`。
6. **契约测试**：用环境变量钩子绕弹窗、直接走 facade 验数据契约（见下）。

新增一个**原子**：放 `atoms/`，登 `atoms/qmldir` + `moganqml.qrc`（`qml/atoms/` 前缀）；
内部 id 用组件语义缩写（不用 `root`）；魔法数字提成 `Theme` 常量或文件内具名属性。

## 测试

三层，从快到慢：

1. **QML 加载测试**（`tests/Plugins/Qt/qml_load_test.cpp`，`xmake b qml_load_test && xmake r
   qml_load_test`）：`setSource` 后断言 `status()==Ready`。不验交互，只挡 import 缺失、语法
   错、id 悬空、context property 误用——改 atoms/弹窗后跑一遍即可回归。新弹窗加一个
   `test_xxx_loads` 用例。
2. **数据契约测试**（scheme 集成，`TeXmacs/tests/<编号>.scm`，`xmake b stem && xmake r <编号>`）：
   模态弹窗 C++ 单测无法 exec 阻塞跑，用环境变量钩子绕弹窗、直接走 facade 验 meta 形状 /
   live 写回 / 快照撤销（字体：`MOGAN_TEST_FONT_SELECTOR=ok|cancel`，表单：
   `MOGAN_TEST_FORM_DIALOG`，段落/确认关闭同理）。`MOGAN_TEST_GUI=1` 去 headless 在真实
   GUI 跑交互链。
3. **手动 GUI 验证**：真实点选/双击/Esc/Cancel 等交互无法在 scheme 测试触及，靠打开弹窗
   观察确认。

`gf fmt` 只格式化 `.scm`，不处理 `.qml`——QML 改动靠加载测试 + 人工 review。

## 相关

- `src/Plugins/Qt/qml/atoms/` — 原子板块
- `src/Plugins/Qt/qml/` — 成品弹窗（ConfirmClose/FormDialog/FontSelector/ParagraphFormat）
- `src/Plugins/Qt/moganqml.qrc` — 逐文件登记 qml（无 glob，新增/移动须同步）
- `src/Plugins/Qt/QTMQmlDialog.cpp` — 模态引擎 + 各对话框 glue 入口
- `src/Plugins/Qt/QTMQmlDialogBridge.hpp` — `QmlDialogBridge`（choose/submit/cancel 回流）
- `src/Plugins/Qt/FontSelectorBridge.*` — live + 快照撤销 bridge 样板
- `tests/Plugins/Qt/qml_load_test.cpp` — QML 加载测试（新增弹窗在此加用例）
- `ai-docs/qml/qml-dialog.html` — 设计稿
