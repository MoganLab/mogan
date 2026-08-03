import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 520
    implicitHeight: 450
    implicitMargins: Theme.margin

    property string title: typeof recentSearchBridge !== "undefined" ? recentSearchBridge.title : "Search recent documents"
    property string placeholder: typeof recentSearchBridge !== "undefined" ? recentSearchBridge.placeholder : "Search"
    property string emptyText: typeof recentSearchBridge !== "undefined" ? recentSearchBridge.emptyText : "No matching recent documents"
    property var documents: typeof recentSearchBridge !== "undefined" ? recentSearchBridge.documents : []
    property var buttonLabels: typeof recentSearchBridge !== "undefined" ? recentSearchBridge.buttonLabels : ["Open", "Cancel"]
    property var matches: []
    property int selectedIndex: -1

    function fuzzyScore(text, query) {
        var haystack = text.toLocaleLowerCase()
        var needle = query.toLocaleLowerCase()
        var cursor = 0
        var score = 0
        for (var i = 0; i < needle.length; ++i) {
            var index = haystack.indexOf(needle.charAt(i), cursor)
            if (index < 0)
                return -1
            score += index - cursor
            cursor = index + 1
        }
        return score
    }

    function updateMatches() {
        var query = searchInput.text.trim()
        var next = []
        for (var i = 0; i < documents.length; ++i) {
            var document = documents[i]
            var score = query.length === 0 ? i : fuzzyScore(document.name, query)
            if (score < 0 && query.length > 0)
                score = fuzzyScore(document.path, query)
            if (score >= 0)
                next.push({ "name": document.name, "path": document.path, "score": score })
        }
        next.sort(function(a, b) { return a.score - b.score })
        matches = next
        selectedIndex = -1
    }

    function openSelection() {
        if (selectedIndex >= 0 && selectedIndex < matches.length)
            closeBridge.submit({ "path": matches[selectedIndex].path })
    }

    Component.onCompleted: updateMatches()
    onDocumentsChanged: updateMatches()

    content: Column {
        spacing: Theme.gapM

        Text {
            width: parent.width
            height: Theme.titleH
            text: root.title
            color: Theme.fg
            font.pixelSize: 18 * Theme.scaleFactor
            font.weight: Font.Bold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Rectangle {
            width: parent.width
            height: Theme.rowH
            radius: Theme.radius
            color: Theme.fieldBg
            border.width: Theme.borderW
            border.color: searchInput.activeFocus ? Theme.dropdownBorder : Theme.borderClr

            Text {
                anchors.fill: parent
                anchors.leftMargin: Theme.comboPad
                anchors.rightMargin: Theme.comboPad
                text: root.placeholder
                color: Theme.muted
                font.pixelSize: Theme.fontBody
                verticalAlignment: Text.AlignVCenter
                visible: searchInput.text.length === 0 && !searchInput.activeFocus
                elide: Text.ElideRight
            }

            TextInput {
                id: searchInput
                anchors.fill: parent
                anchors.leftMargin: Theme.comboPad
                anchors.rightMargin: Theme.comboPad
                color: Theme.fg
                font.pixelSize: Theme.fontBody
                verticalAlignment: TextInput.AlignVCenter
                selectByMouse: true
                clip: true
                focus: true
                onTextChanged: root.updateMatches()
                Component.onCompleted: forceActiveFocus()
            }
        }

        Rectangle {
            width: parent.width
            height: 230 * Theme.scaleFactor
            radius: Theme.radius
            color: Theme.listBg
            border.width: Theme.borderW
            border.color: Theme.borderClr

            Rectangle {
                anchors.fill: parent
                anchors.margins: parent.border.width
                radius: Math.max(0, parent.radius - parent.border.width)
                color: parent.color
                clip: true

                ListView {
                    id: resultList
                    anchors.fill: parent
                    anchors.margins: Theme.padS
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: root.matches

                    delegate: Rectangle {
                        width: resultList.width
                        height: 48 * Theme.scaleFactor
                        radius: Theme.radius
                        color: root.selectedIndex === index ? Theme.selectBg : (resultMouse.containsMouse ? Theme.fieldBgHover : "transparent")
                        border.width: root.selectedIndex === index ? Theme.borderW : 0
                        border.color: Theme.selectBorder

                        Column {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.comboPad
                            anchors.rightMargin: Theme.comboPad
                            spacing: Theme.padS

                            Text {
                                width: parent.width
                                text: modelData.name
                                color: root.selectedIndex === index ? Theme.selectFg : Theme.fg
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }

                            Text {
                                width: parent.width
                                text: modelData.path
                                color: Theme.muted
                                font.pixelSize: Theme.fontTiny
                                elide: Text.ElideMiddle
                            }
                        }

                        MouseArea {
                            id: resultMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.selectedIndex = index
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: root.matches.length === 0
                    text: root.emptyText
                    color: Theme.muted
                    font.pixelSize: Theme.fontBody
                }
            }
        }

        DialogButtons {
            anchors.horizontalCenter: parent.horizontalCenter
            buttonLabels: root.buttonLabels
            onClicked: function(index) {
                if (index === 0)
                    root.openSelection()
                else
                    closeBridge.cancel()
            }
        }
    }
}
