// StyleCard.qml — 启动页文档样式卡片原子（图标模式 / 缩略图模式）。
// 对应 C++ StyleCard::setupIconMode / setupThumbnailMode。
//
// API:
//   kind      : string — "icon"（新建/打开）或 "thumbnail"（推荐模板）。
//   iconSrc   : string — 图标模式下 SVG 路径（qrc 或内联 data URI），仅 kind=="icon"。
//   cardName  : string — 底部名称文案（图标模式）或缩略图外名称（缩略图模式）。
//   titleText : string — 缩略图内标题栏文案，仅 kind=="thumbnail"。
//   thumbSrc  : string — 缩略图图片 URL，仅 kind=="thumbnail"。
//   clicked()          — 点击信号。

import QtQuick

Rectangle {
    id: card
    width: StartupTheme.cardW
    height: StartupTheme.cardH
    radius: StartupTheme.cardRadius
    color: StartupTheme.cardBg
    border.width: 1
    border.color: mouseArea.containsMouse ? StartupTheme.cardHoverBorder : StartupTheme.cardBorder

    property string kind: "icon"       // "icon" | "thumbnail"
    property string iconSrc: ""        // SVG source for icon mode
    property string cardName: ""       // bottom name
    property string titleText: ""      // thumbnail title bar (thumbnail mode)
    property string thumbSrc: ""       // thumbnail image URL (thumbnail mode)

    signal clicked()

    Behavior on border.color { ColorAnimation { duration: 150 } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: card.clicked()
    }

    // ---- 图标模式 ----
    Loader {
        anchors.fill: parent
        active: card.kind === "icon"
        sourceComponent: Item {
            anchors.fill: parent

            Image {
                id: iconImg
                source: card.iconSrc
                width: StartupTheme.cardIconSize
                height: StartupTheme.cardIconSize
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -12
                fillMode: Image.PreserveAspectFit
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 24 * StartupTheme.scaleFactor
                text: card.cardName
                color: StartupTheme.cardNameFg
                font.pixelSize: StartupTheme.fontCardName
            }
        }
    }

    // ---- 缩略图模式 ----
    Loader {
        anchors.fill: parent
        active: card.kind === "thumbnail"
        sourceComponent: Item {
            anchors.fill: parent

            // 内框 QFrame#style-card-frame: 152×219, margin 4
            Rectangle {
                id: frame
                x: StartupTheme.cardFramePad
                y: StartupTheme.cardFramePad
                width: StartupTheme.cardFrameW
                height: StartupTheme.cardFrameH
                radius: StartupTheme.cardFrameRadius
                color: StartupTheme.cardBg
                border.width: 1
                border.color: mouseArea.containsMouse ? StartupTheme.cardHoverBorder : StartupTheme.cardBorder

                // 缩略图区域 QLabel#style-card-thumbnail: bg #f5f5f5
                Rectangle {
                    id: thumbArea
                    anchors { top: parent.top; left: parent.left; right: parent.right }
                    height: parent.height - StartupTheme.cardTitleH
                    color: StartupTheme.thumbnailBg

                    Image {
                        anchors.fill: parent
                        source: card.thumbSrc
                        fillMode: Image.PreserveAspectFit
                        visible: card.thumbSrc !== ""
                    }
                    Text {
                        anchors.centerIn: parent
                        text: "Thumbnail"
                        color: "#aaa"
                        font.pixelSize: 12 * StartupTheme.scaleFactor
                        visible: card.thumbSrc === ""
                    }
                }

                // 标题栏 QLabel#style-card-title: 29px, bg #fff, color #2b3b45
                Rectangle {
                    anchors { top: thumbArea.bottom; left: parent.left; right: parent.right }
                    height: StartupTheme.cardTitleH
                    color: StartupTheme.cardTitleBg

                    Text {
                        anchors.centerIn: parent
                        text: card.titleText
                        color: StartupTheme.cardTitleFg
                        font.pixelSize: StartupTheme.fontCardTitle
                    }
                }
            }

            // 卡片外名称（缩略图模式在框架下方显示名称）
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 4 * StartupTheme.scaleFactor
                text: card.cardName
                color: StartupTheme.cardNameFg
                font.pixelSize: StartupTheme.fontCardName
            }
        }
    }
}
