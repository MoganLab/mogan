// StartupTheme.qml — Startup tab 全局主题单例（pragma Singleton）。
// 与 dialog Theme.qml 互补：dialog 弹窗走 Theme，startup tab 走 StartupTheme。
// scaleFactor/dark 取自 C++ context property dpScale/isDark；缺省回退 1.0/false。
//
// 用法：import "atoms" 后 StartupTheme.sidebarBg / StartupTheme.scaleFactor 等。

pragma Singleton
import QtQuick

QtObject {
    property real scaleFactor: typeof dpScale !== "undefined" ? dpScale : 1.0
    property bool dark: typeof isDark !== "undefined" ? isDark : false

    // ---- 侧边栏 (liii.css #startup-tab-sidebar: #215a6a) ----
    readonly property color sidebarBg: "#215a6a"
    readonly property color navTitleFg: Qt.rgba(1, 1, 1, 0.6)
    readonly property color navBtnFg: "#ffffff"
    readonly property color navBtnHoverBg: Qt.rgba(1, 1, 1, 0.15)
    readonly property color navBtnActiveBg: "#2791ad"
    readonly property color quitBtnFg: "#ffffff"
    readonly property color quitBtnBorder: Qt.rgba(1, 1, 1, 0.5)
    readonly property color quitBtnHoverBg: Qt.rgba(1, 1, 1, 0.2)

    // ---- 内容区 (liii.css #startup-tab-content: #f3f3f3) ----
    readonly property color contentBg: dark ? "#2b2b2b" : "#f3f3f3"
    readonly property color sectionTitleFg: dark ? "#4db6ac" : "#215a6a"

    // ---- StyleCard (liii.css QWidget#style-card) ----
    readonly property color cardBg: dark ? "#3d3d3d" : "#ffffff"
    readonly property color cardBorder: dark ? "#4e4e4e" : "#cfcfcf"
    readonly property color cardHoverBorder: "#2791ad"
    readonly property color cardIconFg: dark ? "#4db6ac" : "#215a6a"
    readonly property color cardNameFg: dark ? "#e0e0e0" : "#2b3b45"
    readonly property color cardTitleBg: dark ? "#3d3d3d" : "#ffffff"
    readonly property color cardTitleFg: dark ? "#e0e0e0" : "#2b3b45"
    readonly property color cardTitleBorderTop: dark ? "#4e4e4e" : "#f0f0f0"
    readonly property color thumbnailBg: dark ? "#333333" : "#f5f5f5"

    // ---- 分隔线 (liii.css QFrame#startup-tab-separator: #d8dde2) ----
    readonly property color separatorColor: dark ? "#3e3e3e" : "#d8dde2"

    // ---- 最近文档 (liii.css QListWidget#recent-docs-list) ----
    readonly property color recentBg: dark ? "#3d3d3d" : "#ffffff"
    readonly property color recentBorder: dark ? "#4e4e4e" : "#e0e0e0"
    readonly property color recentNameFg: dark ? "#eaeaea" : "#1f2933"
    readonly property color recentTimeFg: "#888888"
    readonly property color recentHoverBg: dark ? "#4e4e4e" : "#f0f0f0"
    readonly property color recentEmptyFg: "#999999"

    // ---- 模板卡片 (liii.css QFrame#startup-tab-template-card) ----
    readonly property color templateCardBg: dark ? "#3d3d3d" : "#ffffff"
    readonly property color templateCardBorder: dark ? "#4e4e4e" : "#cfcfcf"
    readonly property color templateNameFg: dark ? "#e0e0e0" : "#333333"
    readonly property color templateInfoFg: "#888888"

    // ---- 尺寸 (96 DPI 逻辑值, 乘 scaleFactor) ----
    // 侧边栏
    readonly property real sidebarWidth: 160 * scaleFactor
    readonly property real navBtnPadV: 8 * scaleFactor
    readonly property real navBtnPadH: 12 * scaleFactor
    readonly property real navBtnRadius: 4 * scaleFactor
    readonly property real quitBtnRadius: 4 * scaleFactor

    // 内容区内边距 (HTML: padding 76px 48px 40px, 其中 76px 含标题栏 44px,
    // startup tab 不含标题栏故上边距用 32px)
    readonly property real contentPadTop: 32 * scaleFactor
    readonly property real contentPadH: 48 * scaleFactor
    readonly property real contentPadBottom: 40 * scaleFactor

    // StyleCard
    readonly property real cardW: 160 * scaleFactor
    readonly property real cardH: 228 * scaleFactor
    readonly property real cardRadius: 8 * scaleFactor
    readonly property real cardIconSize: 50 * scaleFactor
    readonly property real cardFrameW: 152 * scaleFactor
    readonly property real cardFrameH: 219 * scaleFactor
    readonly property real cardFramePad: 4 * scaleFactor
    readonly property real cardTitleH: 29 * scaleFactor
    readonly property real cardFrameRadius: 6 * scaleFactor

    // TemplateCard (HTML: 176×243 缩略图区 + 名称 + 信息)
    readonly property real tplCardW: 176 * scaleFactor
    readonly property real tplCardH: 243 * scaleFactor
    readonly property real tplThumbW: 160 * scaleFactor
    readonly property real tplThumbH: 227 * scaleFactor
    readonly property real tplThumbRadius: 2 * scaleFactor

    // 最近文档 (HTML: item min-height 40px, radius 4px, margin 2px 4px, padding 6px 14px)
    readonly property real recentItemH: 40 * scaleFactor
    readonly property real recentItemPadV: 6 * scaleFactor
    readonly property real recentItemPadH: 14 * scaleFactor
    readonly property real recentItemMarginV: 2 * scaleFactor
    readonly property real recentItemMarginH: 4 * scaleFactor
    readonly property real recentRadius: 8 * scaleFactor
    readonly property real recentItemRadius: 4 * scaleFactor

    // ---- 字号 ----
    readonly property real fontSectionTitle: 16 * scaleFactor
    readonly property real fontNavTitle: 11 * scaleFactor
    readonly property real fontNavBtn: 13 * scaleFactor
    readonly property real fontCardName: 14 * scaleFactor
    readonly property real fontCardTitle: 10 * scaleFactor
    readonly property real fontRecentName: 15 * scaleFactor
    readonly property real fontRecentTime: 11 * scaleFactor
    readonly property real fontRecentEmpty: 13 * scaleFactor
    readonly property real fontTemplateName: 11 * scaleFactor
    readonly property real fontTemplateInfo: 10 * scaleFactor
    readonly property real fontTemplateLoading: 14 * scaleFactor

    // ---- 间距 ----
    readonly property real gapCards: 16 * scaleFactor
    readonly property real sectionGap: 20 * scaleFactor
}
