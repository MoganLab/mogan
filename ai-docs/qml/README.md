# QML 对话框

`src/Plugins/Qt/qml/` 的弹窗体系。新增对话框先读这份，避免踩已知的坑。

## 板块

**原子**（拼装用，勿自造外壳/配色）：
- `DialogShell` — 外壳：圆角、无边框拖动、Esc、`content` 正文槽、共享下拉浮层
- `DialogButtons` — 按钮行，只发 `clicked(index)`，语义由调用方映射
- `EnumCombo` — 下拉 combo 行（浮层由 DialogShell 管）
- `SelectableList` — 常驻单选列表（family/style/size 三栏用）
- `PreviewPane` — 预览图区（显示 bridge 光栅化的 PNG data URL）
- `Theme` — 主题单例（scaleFactor / 暗色 / 配色）

**成品**：
| 对话框 | 模态 | 模型 |
|--------|------|------|
| `ConfirmClose` | 模态 | 点按钮返回结果 |
| `FormDialog` | 模态 | 本地暂存 `values`，OK 一次性 submit |
| `FontSelector` | **非模态** | live 写回文档，OK 落定 / Cancel 回滚 |

## 模态 vs 非模态（最重要的坑）

底座在 `QTMQmlDialog.cpp`：`run_qml_dialog`（模态 `exec`）/ `show_qml_dialog`（非模态 `show`）。

**需要 live 写回文档、实时看到效果的对话框，必须用非模态 `show_qml_dialog`。**
模态 `exec` 的嵌套事件循环**不重绘被遮的文档窗口**——字体选择器最初用模态，
scheme 改了文档树但画面不更新，要点 OK 关窗才看到。main 的老 side-tool 非模态故无此问题。

非模态生命期（`show_qml_dialog` 约定）：QDialog 堆分配 + `WA_DeleteOnClose`；
bridge 不挂 parent，靠 `connect(host, destroyed, bridge, deleteLater)` 自清；
OK/Cancel 调 `host->close()`（不是 `done()`，非模态下 done 不关窗）。样板见
`cpp_font_selector_dialog`。

## 测试

非模态对话框 C++ 单测无法 `exec` 阻塞跑，用环境变量钩子绕过弹窗、直接走 facade
验数据契约（字体选择器：`MOGAN_TEST_FONT_SELECTOR=ok|cancel`）。

## 相关

- `src/Plugins/Qt/QTMQmlDialog.cpp` — 模态/非模态引擎 + 各对话框 glue 入口
- `src/Plugins/Qt/QTMQmlDialogBridge.hpp` — `QmlDialogBridge`（choose/submit 回流）
- `src/Plugins/Qt/FontSelectorBridge.*` — 非模态 bridge 样板
- `ai-docs/qml/qml-dialog.html` — 设计稿
