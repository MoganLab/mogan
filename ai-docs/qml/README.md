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

**原子**（拼装用，勿自造外壳/配色）：
- `DialogShell` — 外壳：圆角、无边框拖动、Esc、`content` 正文槽、共享下拉浮层
- `DialogButtons` — 按钮行，只发 `clicked(index)`，语义由调用方映射
- `MiniButton` — 紧凑小按钮（正文内辅助按钮组，如行间距预设），只发 `clicked()`
- `EnumCombo` — 下拉 combo 行（浮层由 DialogShell 管），支持 optionsTr 翻译分离
- `EnumComboList` — 可滚动的 EnumCombo 竖列（Filter/Advanced 选项卡内容）
- `SelectableList` — 常驻单选列表（family/style/size 三栏用）
- `PreviewPane` — 预览图区（显示 bridge 光栅化的 PNG data URL）
- `TabBar` — 胶囊选项卡行
- `TabPanel` — 带选项卡的容器面板（TabBar + content 槽）
- `Theme` — 主题单例（scaleFactor / 暗色 / 配色）

**成品**：
| 对话框 | 模态引擎 | 模型 |
|--------|---------|------|
| `ConfirmClose` | `run_qml_dialog`（exec） | 点按钮返回结果 |
| `FormDialog` | `run_qml_dialog`（exec） | 本地暂存 `values`，OK 一次性 submit |
| `FontSelector` | `run_modal_qml_dialog`（setModal+show） | live 写回文档，OK 落定 / Cancel+重置快照撤销 |

## 模态引擎（QTMQmlDialog.cpp）

两个引擎都收敛了 QDialog 拼装 + setSource + 加载检查 + 定尺寸流程，差异只在 exec vs
setModal+show（见上表）。新增模态对话框写 context 注入回调 + 解读返回值即可，不重抄
宿主拼装。

## 测试

模态对话框 C++ 单测无法 exec 阻塞跑，用环境变量钩子绕过弹窗、直接走 facade 验数据契约
（字体选择器：`MOGAN_TEST_FONT_SELECTOR=ok|cancel`，表单：`MOGAN_TEST_FORM_DIALOG`）。

## 相关

- `src/Plugins/Qt/QTMQmlDialog.cpp` — 模态引擎 + 各对话框 glue 入口
- `src/Plugins/Qt/QTMQmlDialogBridge.hpp` — `QmlDialogBridge`（choose/submit 回流）
- `src/Plugins/Qt/FontSelectorBridge.*` — live + 快照撤销 bridge 样板
- `ai-docs/qml/qml-dialog.html` — 设计稿
