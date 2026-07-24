// TemplateCard.qml — 模板分类页卡片原子。
// 对应 C++ QTMTemplatePage::createTemplateCard。
// 176×243 缩略图区 + 名称 + 作者·版本信息。
//
// API:
//   templateId   : string — 模板 ID（clicked 信号携带）。
//   name         : string — 模板名称（11px, 最多两行）。
//   author       : string — 作者名。
//   version      : string — 版本号。
//   thumbnailUrl : string — 缩略图 URL。
//   clicked(string templateId) — 点击信号。

import QtQuick

Rectangle {
    id: root
    width: cardFrame.width
    height: cardFrame.height + nameLabel.height + infoLabel.height + 8 * StartupTheme.scaleFactor
    color: "transparent"

    property string templateId: ""
    property string name: ""
    property string author: ""
    property string version: ""
    property string thumbnailUrl: ""

    signal clicked(string templateId)

    // 缩略图卡片 QFrame#startup-tab-template-card: 176×243
    Rectangle {
        id: cardFrame
        width: StartupTheme.tplCardW
        height: StartupTheme.tplCardH
        radius: StartupTheme.cardRadius
        color: StartupTheme.templateCardBg
        border.width: 1
        border.color: rootMouse.containsMouse ? StartupTheme.cardHoverBorder : StartupTheme.templateCardBorder

        Behavior on border.color { ColorAnimation { duration: 150 } }

        // 缩略图内框 (HTML: .tpl-thumb-inner, 160×227, bg #f5f5f5, radius 2px)
        Rectangle {
            id: thumbInner
            anchors.centerIn: parent
            width: StartupTheme.tplThumbW
            height: StartupTheme.tplThumbH
            radius: StartupTheme.tplThumbRadius
            color: StartupTheme.thumbnailBg

            Image {
                anchors.fill: parent
                source: root.thumbnailUrl
                fillMode: Image.PreserveAspectFit
                visible: root.thumbnailUrl !== ""
            }
            Text {
                anchors.centerIn: parent
                text: root.name
                color: "#aaa"
                font.pixelSize: 12 * StartupTheme.scaleFactor
                visible: root.thumbnailUrl === ""
                elide: Text.ElideRight
                maximumLineCount: 2
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // 模板名称 (HTML: .tpl-name, width 176px, margin-top 5px, font 11px, color #333)
    Text {
        id: nameLabel
        anchors { top: cardFrame.bottom; topMargin: 5 * StartupTheme.scaleFactor; horizontalCenter: parent.horizontalCenter }
        width: StartupTheme.tplCardW
        text: root.name
        color: StartupTheme.templateNameFg
        font.pixelSize: StartupTheme.fontTemplateName
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        maximumLineCount: 2
    }

    // 作者·版本 (HTML: .tpl-info, margin-top 1px, font 10px, color #888)
    Text {
        id: infoLabel
        anchors { top: nameLabel.bottom; topMargin: 1 * StartupTheme.scaleFactor; horizontalCenter: parent.horizontalCenter }
        text: {
            var parts = []
            if (root.author) parts.push(root.author)
            if (root.version) parts.push("v" + root.version)
            return parts.length > 0 ? parts.join(" · ") : ""
        }
        color: StartupTheme.templateInfoFg
        font.pixelSize: StartupTheme.fontTemplateInfo
        visible: text !== ""
    }

    // 点击区域覆盖整个组件（包括名称和版本信息）
    MouseArea {
        id: rootMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked(root.templateId)
    }
}
