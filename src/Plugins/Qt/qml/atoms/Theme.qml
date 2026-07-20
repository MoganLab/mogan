// Theme.qml — 全局主题单例（pragma Singleton）。
// 供所有 QML 弹窗与原子板块共享 scaleFactor / 暗色 / 主题色，取代各文件各存一份配色。
// scaleFactor/dark 取自 C++ 注入的 context property dpScale/isDark（QQmlContext 对
// engine 级 singleton 可见）；缺省回退（独立预览/单测不注入时不崩）。
//
// 用法：成品弹窗 qml 顶部 `import "atoms"` 后直接 `Theme.bg` / `Theme.scaleFactor` 等。

pragma Singleton
import QtQuick

QtObject {
    property real scaleFactor: typeof dpScale !== "undefined" ? dpScale : 1.0
    property bool dark: typeof isDark !== "undefined" ? isDark : false

    readonly property color bg: dark ? "#2b2b2b" : "#ffffff"          // 弹窗背景
    readonly property color fg: dark ? "#eaeaea" : "#1e1e1e"          // 正文/标签文字
    readonly property color accent: dark ? "#7a7a7a" : "#1e1e1e"      // 主按钮底色
    readonly property color fieldBg: dark ? "#3d3d3d" : "#f1f3f4"     // 输入框/下拉/combo 底
    readonly property color fieldBgHover: dark ? "#4e4e4e" : "#e5e7eb"// 同上 hover
    readonly property color borderClr: dark ? "#3e3e3e" : "#d0d4da"   // 边框

    // 列表项选中态
    readonly property color selectBg: dark ? "#1f4a48" : "#dff3f1"     // 选中项底色
    readonly property color selectBorder: dark ? "#2f6a67" : "#b3d9d6" // 选中项边框
    readonly property color selectFg: dark ? "#bfeeeb" : "#194f53"    // 选中项文字

    // 列表/选项卡容器底色
    readonly property color listBg: dark ? "#2a3a39" : "#fbfdfd"

    // 下拉浮层外框（青灰，呼应选中态青色，比通用 borderClr 更明显）
    readonly property color dropdownBorder: dark ? "#3d6a66" : "#a9c9c6"

    // 结构尺寸常量（跨弹窗/原子共用，收敛魔法数字）。
    readonly property real rowH: 44 * scaleFactor   // 字段行高（EnumCombo 行 / ParagraphFormat 行间距预设行）
    readonly property real btnH: 40 * scaleFactor    // 主按钮高度（DialogButtons）
    readonly property real btnW: 100 * scaleFactor   // 主按钮默认宽度（DialogButtons buttonWidth 默认）
    readonly property real itemH: 36 * scaleFactor   // 列表项行高（下拉浮层选项 / SelectableList delegate）
    readonly property real textRowH: 24 * scaleFactor // 纯文本行高（Statistics 统计行等简单 label:value 行）
    readonly property real titleH: 28 * scaleFactor  // 弹窗标题区高度

    // 圆角（胶囊类用 height/2，不在此列）。
    readonly property real radius: 8 * scaleFactor    // 容器/卡片/下拉浮层标准圆角
    readonly property real radiusLg: 16 * scaleFactor // 弹窗外壳大圆角（DialogShell 背景）

    // 字号阶梯（各原子内统一）。
    readonly property real fontBody: 14 * scaleFactor // 正文/标签/列表项字号
    readonly property real fontBtn: 15 * scaleFactor  // 主按钮（DialogButtons）字号
    readonly property real fontTab: 13 * scaleFactor  // 选项卡（TabBar）字号
    readonly property real fontMini: 11 * scaleFactor // 紧凑按钮（MiniButton）字号
    readonly property real fontTiny: 10 * scaleFactor // 箭头/角标等微型字号

    // 间距/边距阶梯（跨原子共用）。
    readonly property real borderW: 1 * scaleFactor   // 边框宽度
    readonly property real padS: 4 * scaleFactor      // 小内边距/间隙（高亮块上下留白、浮层间隙）
    readonly property real pad: 8 * scaleFactor       // 标准内边距/缩进（容器内缩、列表 padding）
    readonly property real gapS: 6 * scaleFactor      // 小行间距（EnumComboList 行间）
    readonly property real gapM: 16 * scaleFactor     // 中间距（按钮间、combo 行 label↔控件）
    readonly property real inlineGap: 12 * scaleFactor // 行内间隙（Statistics label↔value）
    readonly property real margin: 24 * scaleFactor   // 弹窗正文四周大边距（DialogShell implicitMargins）

    // 原子级布局常量（某原子专用，提名为常量避免散落字面量）。
    readonly property real headerH: 24 * scaleFactor  // SelectableList 标题行高
    readonly property real comboPad: 14 * scaleFactor // EnumCombo 文本左内边距
    readonly property real comboArrowGap: 30 * scaleFactor // EnumCombo 右侧箭头预留宽度
    readonly property real arrowMargin: 12 * scaleFactor   // EnumCombo 箭头自身右边距
    readonly property real tabH: 30 * scaleFactor     // TabBar 选项卡高度
    readonly property real tabPad: 28 * scaleFactor   // TabBar 选项卡左右 padding（text 外加宽）
    readonly property real listTextPadL: 20 * scaleFactor // SelectableList 列表项文本左内边距
    readonly property real listTextPadR: 18 * scaleFactor // SelectableList 列表项文本右内边距
    readonly property real miniBtnW: 48 * scaleFactor // MiniButton 默认宽
    readonly property real miniBtnH: 28 * scaleFactor // MiniButton 默认高
    readonly property real miniBtnR: 7 * scaleFactor  // MiniButton 圆角
}
