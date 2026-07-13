// Theme.qml — 全局主题单例（pragma Singleton）。
// 供所有 QML 弹窗与原子板块共享 scaleFactor / 暗色 / 主题色，取代各文件各存一份配色。
// scaleFactor/dark 取自 C++ 注入的 context property dpScale/isDark（QQmlContext 对
// engine 级 singleton 可见）；缺省回退（独立预览/单测不注入时不崩）。
//
// 用法：弹窗 qml 顶部 `import "."` 后直接 `Theme.bg` / `Theme.scaleFactor` 等。

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

    // 列表项选中态（对齐 HTML .list-box li.active：浅青底 + 边框 + 深青字）。
    readonly property color selectBg: dark ? "#1f4a48" : "#dff3f1"     // 选中项底色
    readonly property color selectBorder: dark ? "#2f6a67" : "#b3d9d6" // 选中项边框
    readonly property color selectFg: dark ? "#bfeeeb" : "#194f53"    // 选中项文字
}
