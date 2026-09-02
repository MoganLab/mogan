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

## Cancel/重置撤销 + 即时显示（live 写回对话框）

live 写回的对话框（FontSelector / ParagraphFormat）需要 Cancel/重置撤销已应用的改动。
**不要用 mark-cancel**（mogan 的 undo mark 事务）——它有两个缺陷：

- 对 init 块改动无效（文档字体走 init-multi 改 init，不进 undo 历史，mark-cancel 回滚不了）
- 对 buffer 改动丢选区（mark-cancel 的 apply 触发 post_notify→selection_cancel）

正解是**快照写回**：对话框打开瞬间（register-specs，live 改动前）快照；Cancel 走快照 revert
（`revert-to-snapshot` / `restore-snapshot`），再 `selector-get-changes` + **一次** setter 写回。
一次 setter 避免多次 make-multi-with 嵌套吞选区；两条 setter 路径（init-multi / make-multi-with）
都适用。**注意 Reset 与 Cancel 语义不同**：Cancel=撤销本对话框改动（回快照）；文档级 Reset=恢复系统默认
（FontSelector 文档级走 `selector-restore specs #t` 的 `:default` 移除 init、ParagraphFormat 文档级走
`init-default`），段落级 Reset 因 `:default` 未实现仍走回快照。**切忌给段落级 setter（make-multi-with）
喂 `:default`**——会生成非法 `(with "font" :default <tree>)`、抛 cpp-insert 错并吞掉选中内容。

### 本地真相表 reader（凡带 reset 的对话框必用）

写回文档树后立刻跨 eval 读 `get-env`/`get-init` 会**滞后一拍**（读取相对编辑命令有延迟）——
表现为「重置点一次没用，点第二次才生效」。正解是给 facade 配一个本地真相表
（`utils/library/dialog-value-table`）：读取经 `value-table-ref` **表优先命中、缺项才 fallback
实时树**；set/reset/Cancel 把目标值写入表，后续读即时生效、不碰树。

**凡是带 reset/Cancel 的 live 写回对话框，必须用此机制**：reset/Cancel 撤销时把目标值（快照值
或 init-default 后的默认值）写入本地表，QML 重读 meta 走表，避开延迟。

API 速查（`utils/library/dialog-value-table`，entry-key 由调用方自定义、模块不关心其形状）：

| API | 何时调 | 作用 |
|-----|--------|------|
| `value-table-set! entry-key val` | register / set / reset / cancel | 记当前值，下次 ref 命中表 |
| `value-table-ref entry-key fallback-proc` | meta（读显示值） | 表优先，缺项才 fallback 实时源 |
| `value-table-remove! entry-key` | reset 单字段 | 删项，回退 fallback |
| `value-table-clean entry-keys` | cleanup（关窗） | 清整组防泄漏 |

**不变量**：凡写文档树的路径（set/reset/cancel）都必须同步 `set!` 本表，否则表值与文档真相背离。
使用者（凡带 reset 的 live 写回对话框必接入）：

- **FontSelector**（`fonts/font-new-widgets.scm`）：entry-key = `(specs var buffer)`，fallback
  为字体专用 `initial-font-data`/`initial-customize-get`；Cancel 经 `font-selector-revert-to-snapshot`
  把快照填表；Reset 按 specs 的 `global?` 分流——文档级走 `selector-restore specs #t` 的 `:default`
  （移除 init、清表，回系统默认），段落级（`:default` 未实现）走 revert-to-snapshot 回快照。
- **ParagraphFormat**（`generic/paragraph-format-widgets.scm`）：entry-key = `(key var)`（key 为 register 返回的实例句柄），
  fallback 为 scope 路由的 `get-env`/`get-init`；register 填表、set 同步写、reset 段落级写快照值/
  文档级 init-default 后写 `get-init` 默认值、Cancel restore-snapshot 写快照值、cleanup 清表。

## Preferences 契约（preferences-qml-meta / PreferencesBridge）

Preferences（首选项）是 FormDialog 模式的变体：**本地暂存 + OK 一次性 diff 提交**。
打开时 `prefBridge.meta()` 一次性拉全部 tab/字段描述符树（打开快照 `initialValues`）；
用户改动只改 QML 本地 `values`（条件锁定 / radio 互斥纯 QML 本地，不往 facade 写）；
OK 时算与快照的 diff → `prefBridge.submit(diff)` 一次性应用；Cancel 丢弃
（`prefBridge.cancel()` 只关窗，scm 侧 no-op）。Preferences.qml 头注释引用本节。

**prefBridge 契约**（`PreferencesBridge`，无状态透传——preferences 是全局的，bridge 不持有偏好数据）：

| 方法 | 返回 / 语义 |
|------|------------|
| `meta()` | `{tabs: [{key, label, fields, subTabs?}]}`（subTabs 与 tab 同构） |
| `submit(changed)` | `"applied"` / `"restart"` / `"later"` / `"cancel"` 四态：scm 先应用 diff；含需重启字段时先弹 `cpp-confirm-restart`，三选一映射 restart / later(保存稍后) / cancel(回退该字段) |
| `cancel()` | 关窗，本地丢弃 |
| `callAction(name)` | 行内按钮动作路由（如 `open-auto-backup-location`，label 与实现由插件注入） |

**field-descriptor 字段**（scm 侧 `preferences-qml-field->descriptor` 产出，assoc-list）：

| 字段 | 说明 |
|------|------|
| `kind` | `combo` / `toggle` / `info`（scm 按 options 非空 / key 空自动分流） |
| `key` | preference 内部键（combo/toggle 必有；info 无、不入可编辑 map） |
| `label` / `hint` / `group` | 已翻译文案（编码注意见下） |
| `value` | wire 格式统一字符串：toggle 为 `"on"/"off"`，combo 为 options 内部键 |
| `options` / `optionsTr` | 内部键 × 翻译显示，等长同序；动态字段（language、scripting language、image format）在 meta 构建期拉取，look and feel 按平台裁剪 |
| `editable` | combo 双击可键入（透传 EnumCombo） |
| `restart` | 改动需重启生效 |
| `radioGroup` | 互斥组：开一则同组其它置 off（QML 本地） |
| `enabledWhenKey` / `enabledWhenVal` | 条件锁定：依赖键等于值才可勾，否则 Toggle 锁定灰显（QML 本地） |
| `layout` / `column` | `"two-col"` 双栏段与 0/1 列号（Mathematics / Experimental） |
| `groupSpan` | 分组标题横跨整行（统领双栏两列，如 IR 遥控组） |
| `buttonLabel` / `buttonAction` | combo 旁行内按钮，点击经 `callAction(buttonAction)` 路由 |

编码注意：scm 字面量是 UTF-8，label/hint/group/buttonLabel 先 `utf8->cork` 再
`translate`，bridge 侧 `cork_to_utf8` 还原——跳过归一化会让含非 ASCII 的文案
（如 `TeXmacs → Html` 的箭头）被二次解码成乱码。偏好键一律走
`pref-keys.scm` 的 `pref-*` 访问器。字段定义与 descriptor 构建在
`TeXmacs/progs/texmacs/menus/preferences-tools.scm`，tab 组织与 submit 在
`preferences-widgets.scm`；契约测试 `TeXmacs/tests/2044.scm`。

## 板块

`src/Plugins/Qt/qml/` 分两层目录：

- **`atoms/`** — 原子板块（拼装用，勿自造外壳/配色），各有 `atoms/qmldir` 登记。
  成品弹窗 `import "atoms"` 取用（含 `Theme` 单例）。
- **`qml/`**（本目录）— 成品弹窗，`qml/qmldir` 登记。

**原子**（`atoms/`，拼装用，勿自造外壳/配色）：
- `DialogShell` — 外壳：圆角、无边框拖动、Esc、`content` 正文槽、共享下拉浮层。
  `onCancel`/`onActivate` 是可覆盖回调（默认 Esc → `closeBridge.cancel()`；FontSelector
  覆盖转调 `fontBridge.cancel()`，UpdaterProgress 覆盖为 no-op 禁 Esc）
- `DialogButtons` — 按钮行，只发 `clicked(index)`，`primaryIndex` 定主按钮配色，
  语义由调用方映射
- `MiniButton` — 正文内辅助按钮，`size` 两档：`"mini"` 紧凑小按钮（行间距预设组）、
  `"normal"` 与 EnumCombo 行等高的行内 action 按钮（宽度按文案自适应），只发 `clicked()`
- `EnumCombo` — 下拉 combo 行（浮层由 DialogShell 管），key/显示分离（`options` 英文键 ×
  `optionsTr` 翻译显示）；`editable: true` 双击进可编辑输入态（Enter/失焦落定、Esc 撤销），
  供数值类字段键入预设外的自定义值；`isNarrow` 双栏半宽列自适应；`actionLabel` 行内按钮
  （发 `actionClicked`，如「打开备份目录」）
- `EnumComboList` — 可滚动的 EnumCombo 竖列，两种取值模式：默认 meta 自带 value
  （FontSelector）；`valueSource` 外部真相源 map 模式——改动只外发 `itemChanged`、由
  调用方更新 map（live 写回且 get-env 重读有延迟的场景用，如段落格式，避免显示滞后一拍）
- `SelectableList` — 常驻单选列表（family/style/size 三栏用），自带 `refreshTick`
  驱动 currentValue 重算；值未变仅发信号时需调用方显式 `syncActiveValue()`
- `PreviewPane` — 预览图区（显示 bridge 光栅化的 PNG data URL）
- `TabBar` — 胶囊选项卡行
- `TabPanel` — 带选项卡的容器面板（TabBar + content 槽；content 与 DialogShell 同样
  由组件 reparent 锚定，调用方勿自行 anchors.fill）
- `GroupHeader` — 分组小标题（加粗 + 对称间距，不画分隔线；`isFirst` 首组不加上间距）
- `Toggle` — 布尔开关行（label + 可选 hint + 右侧 on/off 胶囊，切换滑动动画）；
  `isNarrow` 双栏半宽列缩小字体加大 label 占比；`enabled: false` 锁定灰显
  （enabledWhen 条件锁定用）
- `Theme` — 主题单例（scaleFactor / 暗色 / 配色 / 尺寸与字号阶梯常量，分类规则见
  「编码规矩」）
- `InputField` — 自由输入行（路径/页码/搜索词；可选行内按钮）

**成品**：
| 对话框 | 模态引擎 | 模型 |
|--------|---------|------|
| `ConfirmClose` | `run_qml_dialog`（exec） | 点按钮返回结果 |
| `ConfirmQuestion` | `run_qml_dialog`（exec） | 默认按钮居右的确认弹窗（替换 question-no-cancel 的 QMessageBox），返回语义按钮下标（按钮反序协议见「编码规矩」） |
| `ConfirmRestart` | `run_qml_dialog`（exec） | 三按钮（立即重启/稍后/取消），返回 `"restart"/"later"/"cancel"` |
| `FormDialog` | `run_qml_dialog`（exec） | 本地暂存 `values`，OK 一次性 submit（页面设置走此弹窗） |
| `PrintToFile` | `run_qml_dialog`（exec） | 路径 + 页码一次提交；Browse 走原生保存框 |
| `FontSelector` | `run_modal_qml_dialog`（setModal+show） | live 写回文档，OK 落定 / Cancel 快照撤销 / Reset 按 global? 分流（文档级系统默认、段落级回快照） |
| `ParagraphFormat` | `run_modal_qml_dialog`（setModal+show） | live 写回（段落 with / 文档 initial），按 scope 撤销 |
| `Statistics` | `run_qml_dialog`（exec） | 纯展示统计行（`statsItems` 注入 `{label,value}`），Close 即关，无返回值 |
| `Version` | `run_qml_dialog`（exec + 自适应高度） | `VersionDialogBridge` 只读（title/lines/buttonLabels）+ `confirm()`；单行超宽自动换行，定宽后读 implicitHeight 锁高（见「模态引擎」） |
| `Preferences` | `run_qml_dialog`（exec） | 本地暂存 + OK 一次性 diff 提交（契约见上节）；含需重启字段时先弹 ConfirmRestart |
| `UpdaterProgress` | `run_modal_qml_dialog`（setModal+show） | 更新下载中间态：无进度条只转圈；全局单例宿主（重复 open 不重弹）+ `cpp-updater-dialog-open/close` 成对 glue；Esc 禁用（onCancel 覆盖 no-op）。选 show 模态非为 live 重绘，而是 exec 嵌套循环会阻塞 scheme 的 delayed 轮询链 |

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
- **ConfirmQuestion 按钮反序协议**：scm 传语义序 buttons（`buttons[0]` 为默认）；
  C++ 注入 QML 前反序（默认按钮居右），`dialogPrimary` 指显示序最后一个；QML 回传
  `choose` 为显示下标 +1，C++ 映射回语义下标 `N - choice`。动按钮顺序前先过一遍这条链。
- **偏好键统一走 `pref-keys.scm`**：弹窗读写 preference 的 key 字符串一律在
  `TeXmacs/progs/kernel/texmacs/pref-keys.scm`（单一可信源）声明 `(define-public (pref-...) "...")`，
  调用方在 quasiquote 里用 `,(pref-...)` 引用而非裸字符串（key 改名会断 notify 回调链路）。
  新增弹窗的字段键须补 pref-keys 声明——现有 117 个访问器已覆盖 Page setup / Font
  selector / ParagraphFormat（`pref-par-*`）/ Preferences 各组。
- **绑定到无参 bridge 函数的属性不会自动重算**：bridge 内部状态变化 QML 感知不到，需靠
  外部计数器注入依赖。`SelectableList.refreshTick` 已封装此模式——调用方 currentValue 绑定
  读 `<listId>.refreshTick`，refresh 时 `<listId>.refreshTick++` 即可，勿手写假读样板。
- **`Rectangle` 的 `border` 与 `clip:true` 互斥**：Qt 的 border 以边缘为中心绘制（半内半外），
  同一 Rectangle 既 `border` 又 `clip:true` 会把外侧半像素 border 自身裁掉、几乎不可见
  （叠加 `radius` 的抗锯齿偏移更甚）。要带边框的裁剪容器，拆**双层 Rectangle**：外层只画
  border（不 clip），内层 `clip:true` 容纳子项。下拉浮层（DialogShell overlay）即此结构。
- **scm 侧传给 QML 的用户可见文本一律走 `(translate "key")`**：标签、标题、按钮文案等
   在 scm 构造数据时即用 `translate` 包裹，不要硬编码英文。值（数字、内部 key）不翻译。
  字典条目按最小粒度登记（如 `"character count"` / `"with spaces"`），系统自动拼接
  （`"Character count (with spaces)"` → `"字符数（计空格）"`），不要为每种组合加整条字典。
  三方分工：文案翻译收敛在 scm 构造侧，C++ 仅 `translate_buttons` 对注入的按钮文案兜底，
  QML 内不硬编码用户可见文案（回退文案用 `qsTr`）。
- **`translate` 的三条自动归一化**（实现在 `src/System/Language/dictionary.cpp` 的
  `dictionary_rep::translate`，`qt_translate` 同源）：
  1. **首字母大写折叠**：查表前把首字符转小写再查，命中后把结果首字母大写回。故字典
     一律**小写首字母登记**（`("switch interface theme" "切换界面主题")`），代码里
     `(translate "Switch interface theme")` / `qt_translate("Switch ...")` 均能命中。
     仅首字符 ASCII 折叠，非全串大小写不敏感。
  2. **递归切分拼接**：整串查不到时，按「非字母字符」剥离前后缀、在最后一个非字母非空格
     字符处二分，各段递归 `translate` 再拼接。故带标点/括号的复合短语（统计项、含 `()`
     的标题）拆最小词登记即可，**空格不触发切分**——纯空格短语须整条登记。
  3. **`::` 后缀剥离**：`"File::menu"` 形式递归查 `"File"`，丢弃 `::` 及之后。
- **字典条目按字母序排列**：`TeXmacs/plugins/lang/dic/en_US/zh_CN.scm` 等字典文件，
  新增条目按首字母段插入（大小写混排时以小写为准），保持可检索。
- **跨弹窗复用的布局常量收进 `Theme.qml`**：行高、间距等可能被多个弹窗共用的数字，统一加在
  `atoms/Theme.qml`。优先用已有常量组合（如 `Theme.btnH + Theme.padS * 2`
  表示按钮区高度），避免为单一用途加新常量。成品弹窗只保留本弹窗专属的布局参数（如列宽比
  `labelW`），引用 Theme 常量（`Theme.pad`/`Theme.gapS`/`Theme.inlineGap`/`Theme.textRowH` 等）。
  `Theme.qml` 内常量按段分类：配色 → 结构尺寸 → 圆角 → 字号阶梯 → 间距/边距阶梯 →
  原子级布局常量（combo/toggle/tab/list/mini 各段，某原子专用）→ 双栏布局（`twoCol*`）。
  新常量按类别归段，不另起散段。
- **跨 parent 链查找的 property 不能喂给 binding**：如 EnumCombo 的 `dialogShell`（沿
  parent 链按 objectName 查找）是命令式 property，parent 变化不触发其重算，binding 会在
  创建瞬间读到 null 永久卡住。依赖它的几何（comboX/Y/W）须在展开时用 `updateGeometry()`
  拍快照，不能改成 binding 实时算。

## 模态引擎（QTMQmlDialog.cpp）

两个引擎都收敛了 QDialog 拼装 + setSource + 加载检查 + 定尺寸流程，差异只在 exec vs
setModal+show（见上表）。新增模态对话框写 context 注入回调 + 解读返回值即可，不重抄
宿主拼装。共用 context property 由 `inject_common_context` 注入：`closeBridge`
（`QmlDialogBridge`：choose/cancel/submit/startMove）、`dpScale`（DPI 缩放）、`isDark`
（跟随 tm_style_sheet）。

**选 show 模态（setModal+show）的两种理由**：① live 重绘文档（FontSelector /
ParagraphFormat）；② 弹窗存续期间 scheme 的 `delayed` 轮询链必须继续跑——exec 的嵌套
事件循环会阻塞它（UpdaterProgress：下载链由 scheme 轮询驱动）。除这两种外一律用 exec。

**bridge 生命周期两式**：exec 型在 `exec()` 返回后手动 `delete bridge`（ConfirmClose /
FormDialog / Preferences / Version 等）；show 型 bridge 不挂 parent，靠
`connect(host, destroyed, bridge, deleteLater)` 自清（见开头「生命期」）。需跨函数持有
show 型宿主的（UpdaterProgress 的 open/close 成对 glue），用 `QPointer<QDialog>` 全局
引用防悬垂，关窗走 `host->close()` 而非 `done()`（不触发 WA_DeleteOnClose 析构）。

**自适应高度（autofit_height）**：正文行数不定时（Version 长文案换行），引擎先按
logic_w 锁宽、让 QML 完成换行布局，再读根 `implicitHeight` 锁高（logic_h 仅回退）。
配套坑：QML 里文本宽度须绑「定宽逻辑值」（如 Version 的 `contentW` = implicitWidth ×
scaleFactor − 2×margin），不能绑实际父宽——show 前 C++ 就要读 implicitHeight，此刻布局
宽度尚未就位，绑父宽会量出错误的换行结果。

## 开发工作流（新增弹窗/原子）

新增一个成品弹窗 `Foo.qml`：

1. **设计稿先行**：在 `ai-docs/qml/qml-dialog.html` 加面板，定布局/取值/交互。
2. **QML 正文**：`src/Plugins/Qt/qml/Foo.qml`，根用 `DialogShell`，`import "atoms"` 取
   原子 + `Theme`。根 id 用 `root`；正文引用根属性走 id（content 被 reparent，parent 链
   不可靠）。尺寸/字号尽量复用 `Theme` 常量；拼接布局数字（列宽/间距）可留字面量。
3. **登记**：`qml/qmldir` 加 `Foo 1.0 Foo.qml`；`moganqml.qrc` 加 `<file>qml/Foo.qml</file>`
   （无 glob，漏登则运行期找不到）。新增原子同理放 `atoms/` 并登 `atoms/qmldir`。
4. **bridge + glue**：若需 live 写回或行内动作，仿 `FontSelectorBridge` 写 C++ bridge
   （独立 QObject + `Q_INVOKABLE` 透传 `eval_scheme`，**不继承 `QmlDialogBridge`**，注入为
   context property）。引擎入口写在 `QTMQmlDialog.cpp`（选 `run_qml_dialog` exec 或
   `run_modal_qml_dialog` setModal+show，理由见「模态引擎」节）；**glue 声明登记在
   `src/Scheme/L5/glue_qt.lua`**（C++ 声明侧 `src/Scheme/L5/glue_l5_extra.hpp`）——不在
   `src/Scheme/Glue/` 目录，编辑器方法 glue 才在那里。**若带 reset/Cancel**，
   scheme facade 接入 `utils/library/dialog-value-table` 做本地真相表（见「Cancel/重置撤销」节），
   否则 reset 会滞后一拍。
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
   `test_xxx_loads` 用例。桩类四种：`StubBridge`（closeBridge 占位）、`StubLiveBridge`
   （FontSelector+ParagraphFormat 全空桩）、`PrefStubBridge`（Preferences 最小字段树，
   覆盖 group/two-col/column/sub-tab 渲染分支）、`VersionStubBridge`（只读属性）。
   原子无独立加载用例——经成品弹窗间接覆盖是有意策略，新原子至少要被一个成品的加载
   路径触达。Version 另有 4 个行为用例（打开聚焦、ESC 取消、ESC 无焦点兜底、长行换行）。
2. **数据契约测试**（scheme 集成，`TeXmacs/tests/<编号>.scm`，`xmake b stem && xmake r <编号>`）：
   模态弹窗 C++ 单测无法 exec 阻塞跑，用环境变量钩子绕弹窗、直接走 facade 验 meta 形状 /
   live 写回 / 快照撤销。钩子全表（实现在 `QTMQmlDialog.cpp` 各入口的 `get_env`）：

   | 钩子 | 对话框 | 消费 |
   |------|--------|------|
   | `MOGAN_TEST_CONFIRM_CLOSE` | ConfirmClose | 2021.scm、qt_qml_dialog_test、qml_dialog_test |
   | `MOGAN_TEST_CONFIRM_RESTART` | ConfirmRestart | 2040.scm、2044.scm、preferences-widgets-test.scm |
   | `MOGAN_TEST_CONFIRM_QUESTION` | ConfirmQuestion | qml_dialog_test（C++ 侧） |
   | `MOGAN_TEST_FORM_DIALOG` | FormDialog（页面设置） | 2023.scm、qml_dialog_test |
   | `MOGAN_TEST_FONT_SELECTOR` | FontSelector | 2028.scm、font_selector_bridge_test |
   | `MOGAN_TEST_PARAGRAPH_FORMAT` | ParagraphFormat | 2029.scm |
   | `MOGAN_TEST_VERSION_DIALOG` | Version | 2080.scm |
   | `MOGAN_TEST_PREFERENCES` | Preferences | preferences_bridge_test（C++ 侧；scheme 契约由 2044.scm 直调 facade 覆盖） |

   Statistics / UpdaterProgress 无钩子（纯展示、无返回值分支）。另有 2036.scm
   （ParagraphFormat 重置即时性 + value-table 缓存，facade 直调）。
   `MOGAN_TEST_GUI=1` 去 headless 在真实 GUI 跑交互链。
3. **手动 GUI 验证**：真实点选/双击/Esc/Cancel 等交互无法在 scheme 测试触及，靠打开弹窗
   观察确认。

`gf fmt` 只格式化 `.scm`，不处理 `.qml`——QML 改动靠加载测试 + 人工 review。

## 相关

- `src/Plugins/Qt/qml/atoms/` — 原子板块（12 个）
- `src/Plugins/Qt/qml/` — 成品弹窗（ConfirmClose / ConfirmQuestion / ConfirmRestart /
  FormDialog / FontSelector / ParagraphFormat / Statistics / Version / Preferences /
  UpdaterProgress）
- `src/Plugins/Qt/moganqml.qrc` — 逐文件登记 qml（无 glob，新增/移动须同步）
- `src/Plugins/Qt/QTMQmlDialog.cpp` — 模态引擎 + 各对话框 glue 入口
- `src/Plugins/Qt/QTMQmlDialogBridge.hpp` — `QmlDialogBridge`（choose/submit/cancel 回流）
  与 `QmlDialogEscFilter`（ESC 兜底）
- `src/Plugins/Qt/FontSelectorBridge.*` / `ParagraphFormatBridge.*` / `PreferencesBridge.*` /
  `VersionDialogBridge.*` — 各专用 bridge（独立 QObject 透传样板）
- `src/Scheme/L5/glue_qt.lua` — QML 对话框 glue 声明（cpp-confirm-close 等 11 个入口）
- scheme facade：`fonts/font-new-widgets.scm`（字体）、`generic/paragraph-format-widgets.scm`
  （段落）、`texmacs/menus/preferences-widgets.scm` + `preferences-tools.scm`（首选项）、
  `texmacs/menus/print-widgets.scm`（页面设置 → FormDialog）、`texmacs/texmacs/tm-tools.scm`
  （统计）、`utils/misc/updater.scm`（更新下载 / ConfirmQuestion）、`doc/help-funcs.scm`（版本）
- `tests/Plugins/Qt/qml_load_test.cpp` — QML 加载测试（新增弹窗在此加用例）
- `ai-docs/qml/qml-dialog.html` — 设计稿
