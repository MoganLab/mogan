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

    property var templates: typeof startupBridge !== "undefined" && startupBridge.categoryTemplates ? startupBridge.categoryTemplates : []
    property string categoryName: typeof startupBridge !== "undefined" && startupBridge.activeCategoryName ? startupBridge.activeCategoryName : ""

        Column {
            id: contentCol
            width: parent.width
            spacing: 0

            Text {
                id: titleLabel
                text: page.categoryName || qsTr("Template Center")
                color: StartupTheme.sectionTitleFg
                font.pixelSize: StartupTheme.fontSectionTitle
                font.weight: Font.DemiBold
            }

            Item { width: 1; height: 16 * StartupTheme.scaleFactor }

            // Loading / Empty 状态
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: page.templates.length === 0 ? qsTr("Loading templates...") : ""
                color: "#888"
                font.pixelSize: 14 * StartupTheme.scaleFactor
                visible: page.templates.length === 0
            }

            // 响应式模板网格
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

            Item { width: 1; height: 40 * StartupTheme.scaleFactor }
        }
    }
