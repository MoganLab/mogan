# QML 对话框

`src/Plugins/Qt/qml/` 的弹窗体系。新增对话框先读这份，避免踩已知的坑。

## 规矩：只用模态

**所有 QML 对话框一律用模态 `run_qml_dialog`（`QDialog::exec`），不用非模态 `show`。**

原因：非模态丢失了模态的「独占输入 / 置顶 / 防重复打开 / 防切窗口」语义，要靠各种
hack（输入拦截器、置顶 flag、单例标志……）逐个补回，补不全还出新 bug。字体选择器曾
为「live 重绘」改非模态，结果引入切窗口选区错位、可重复开出多个对话框等问题，最终
回归模态。模态下若 live 不重绘，应查重绘链路根因（见下），而非改非模态。

模态下 live 写回文档不重绘的已知修法：（待补充——若再次遇到，定位 paint 事件为何
不被处理，从根因修，不要退回非模态。）

## 板块

**原子**（拼装用，勿自造外壳/配色）：
- `DialogShell` — 外壳：圆角、无边框拖动、Esc、`content` 正文槽、共享下拉浮层
- `DialogButtons` — 按钮行，只发 `clicked(index)`，语义由调用方映射
- `EnumCombo` — 下拉 combo 行（浮层由 DialogShell 管），支持 optionsTr 翻译分离
- `EnumComboList` — 可滚动的 EnumCombo 竖列（Filter/Advanced 选项卡内容）
- `SelectableList` — 常驻单选列表（family/style/size 三栏用）
- `PreviewPane` — 预览图区（显示 bridge 光栅化的 PNG data URL）
- `TabBar` — 胶囊选项卡行
- `TabPanel` — 带选项卡的容器面板（TabBar + content 槽）
- `Theme` — 主题单例（scaleFactor / 暗色 / 配色）

**成品**：
| 对话框 | 模型 |
|--------|------|
| `ConfirmClose` | 点按钮返回结果 |
| `FormDialog` | 本地暂存 `values`，OK 一次性 submit |
| `FontSelector` | live 写回文档，OK 落定 / Cancel 回滚 |

## 模态引擎（run_qml_dialog）

底座在 `QTMQmlDialog.cpp`：`run_qml_dialog` 收敛了 QDialog 拼装 + setSource + 加载检查
+ 定尺寸 + exec 流程。新增模态对话框只需写 context 注入回调 + 解读 exec 返回值，不重抄
宿主拼装。生命期：host 栈分配覆盖 exec()，bridge 不挂 parent（须跨 host 存活取值），
调用方持指针用完 delete。

## 测试

模态对话框 C++ 单测无法 exec 阻塞跑，用环境变量钩子绕过弹窗、直接走 facade 验数据契约
（字体选择器：`MOGAN_TEST_FONT_SELECTOR=ok|cancel`，表单：`MOGAN_TEST_FORM_DIALOG`）。

## 相关

- `src/Plugins/Qt/QTMQmlDialog.cpp` — 模态引擎 + 各对话框 glue 入口
- `src/Plugins/Qt/QTMQmlDialogBridge.hpp` — `QmlDialogBridge`（choose/submit 回流）
- `src/Plugins/Qt/FontSelectorBridge.*` — live + undo mark 事务 bridge 样板
- `ai-docs/qml/qml-dialog.html` — 设计稿
