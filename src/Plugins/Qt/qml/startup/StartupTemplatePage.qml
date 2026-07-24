// StartupTemplatePage.qml — 启动页模板分类页。
// 对应 C++ QTMTemplatePage：分类标题 + 响应式模板卡片网格。
//
// 数据来源：startupBridge context property（C++ StartupBridge 注入）。
//   - startupBridge.categoryTemplates: [{id, name, author, version, thumbnailUrl}, ...]
//   - startupBridge.activeCategoryName: string
//
// 动作：startupBridge.openTemplate(id) / previewTemplate(id)

import QtQuick
import "atoms"

Flickable {
    id: page
    contentHeight: contentCol.height
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    // 是否正在加载（跟随 bridge.categoryLoading）
    property bool loading: typeof startupBridge !== "undefined" && startupBridge.categoryLoading ? startupBridge.categoryLoading : false
    // 模板数据（来自 bridge，fallback 为空）
    property var templates: typeof startupBridge !== "undefined" && startupBridge.categoryTemplates ? startupBridge.categoryTemplates : []
    property string categoryName: typeof startupBridge !== "undefined" && startupBridge.activeCategoryName ? startupBridge.activeCategoryName : ""

    Column {
        id: contentCol
        anchors {
            left: parent.left
            leftMargin: StartupTheme.contentPadH
            right: parent.right
            rightMargin: StartupTheme.contentPadH
        }
        spacing: 0

        // 顶部间距（对齐 HTML content padding-top）
        Item { width: 1; height: StartupTheme.contentPadTop }

        // 分类标题 (HTML: .section-title)
        Text {
            id: titleLabel
            text: page.categoryName || qsTr("Template Center")
            color: StartupTheme.sectionTitleFg
            font.pixelSize: StartupTheme.fontSectionTitle
            font.weight: Font.DemiBold
        }

        Item { width: 1; height: 16 * StartupTheme.scaleFactor }

        // Loading 状态
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Loading templates...")
            color: "#888"
            font.pixelSize: StartupTheme.fontTemplateLoading
            visible: page.loading && page.templates.length === 0
        }

        // Empty 状态（加载完成但无模板）
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("No templates in this category")
            color: "#888"
            font.pixelSize: StartupTheme.fontTemplateLoading
            visible: !page.loading && page.templates.length === 0
        }

        // 响应式模板网格 (HTML: .template-grid, auto-fill minmax(176px, 1fr))
        Grid {
            id: templateGrid
            width: parent.width
            spacing: 16 * StartupTheme.scaleFactor
            columns: Math.max(1, Math.floor(parent.width / (StartupTheme.tplCardW + 16 * StartupTheme.scaleFactor)))

            Repeater {
                model: page.templates
                delegate: TemplateCard {
                    templateId: modelData.id
                    name: modelData.name
                    author: modelData.author
                    version: modelData.version
                    thumbnailUrl: modelData.thumbnailUrl || ""
                    onClicked: function(tid) {
                        if (typeof startupBridge !== "undefined") startupBridge.previewTemplate(tid)
                    }
                }
            }
        }

        // 底部间距
        Item { width: 1; height: StartupTheme.contentPadBottom }
    }
}
