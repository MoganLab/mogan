// StartupTemplatePage.qml — 启动页模板分类页。
// GridView 按需创建 delegate（虚拟滚动），避免大量模板时一次性创建全部卡片。

import QtQuick
import "atoms"

Item {
    id: page

    // 数据来源：startupBridge context property（C++ StartupBridge 注入）。
    property bool loading: typeof startupBridge !== "undefined" && startupBridge.categoryLoading ? startupBridge.categoryLoading : false
    property var templates: typeof startupBridge !== "undefined" && startupBridge.categoryTemplates ? startupBridge.categoryTemplates : []
    property string categoryName: typeof startupBridge !== "undefined" && startupBridge.activeCategoryName ? startupBridge.activeCategoryName : ""

    // 顶部标题 + 状态提示
    Column {
        id: header
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        anchors.leftMargin: StartupTheme.contentPadH
        anchors.rightMargin: StartupTheme.contentPadH
        anchors.topMargin: StartupTheme.contentPadTop
        spacing: 0

        Text {
            id: titleLabel
            width: parent.width
            text: page.categoryName || StartupTheme.tr("Template Center")
            color: StartupTheme.sectionTitleFg
            font.pixelSize: StartupTheme.fontSectionTitle
            font.weight: Font.DemiBold
        }

        Item {
            width: 1
            height: 16 * StartupTheme.scaleFactor
        }

        // Loading 状态
        Text {
            width: parent.width
            text: StartupTheme.tr("Loading templates...")
            color: "#888"
            font.pixelSize: StartupTheme.fontTemplateLoading
            visible: page.loading && page.templates.length === 0
        }

        // Empty 状态
        Text {
            width: parent.width
            text: StartupTheme.tr("No templates in this category")
            color: "#888"
            font.pixelSize: StartupTheme.fontTemplateLoading
            visible: !page.loading && page.templates.length === 0
        }
    }

    // 模板网格 — GridView 只创建可见区域的 delegate
    GridView {
        id: templateGrid
        anchors {
            left: parent.left
            right: parent.right
            top: header.bottom
            bottom: parent.bottom
            leftMargin: StartupTheme.contentPadH
            rightMargin: StartupTheme.contentPadH
            topMargin: 0
            bottomMargin: StartupTheme.contentPadBottom
        }
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        model: page.templates

        // 卡片尺寸 + 间距
        cellWidth: StartupTheme.tplCardW + 16 * StartupTheme.scaleFactor
        cellHeight: StartupTheme.tplCardH + 60 * StartupTheme.scaleFactor

        delegate: TemplateCard {
            templateId: modelData.id
            name: modelData.name
            author: modelData.author
            version: modelData.version
            thumbnailUrl: modelData.thumbnailUrl || ""
            onClicked: function (tid) {
                if (typeof startupBridge !== "undefined")
                    startupBridge.previewTemplate(tid);
            }
        }

        // 新 delegate 淡入，避免切换分类时完全空白
        populate: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 120
            }
        }
    }
}
