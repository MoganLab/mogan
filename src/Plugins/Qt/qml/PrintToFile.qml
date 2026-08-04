// PrintToFile.qml - QML workflow for choosing a printable output file.

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 500
    implicitHeight: 430

    property var defaults: typeof printDefaults !== "undefined" ? printDefaults : ({})
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["Print", "Cancel"]
    property string titleText: typeof printTitle !== "undefined" ? printTitle : "Print to file"
    property string fileText: typeof fileLabel !== "undefined" ? fileLabel : "File:"
    property string formatText: typeof formatLabel !== "undefined" ? formatLabel : "Format:"
    property string pagesText: typeof pagesLabel !== "undefined" ? pagesLabel : "Pages:"
    property string allPagesText: typeof allPagesLabel !== "undefined" ? allPagesLabel : "All pages"
    property string pageRangeText: typeof pageRangeLabel !== "undefined" ? pageRangeLabel : "Page range"
    property string fromText: typeof fromLabel !== "undefined" ? fromLabel : "From"
    property string toText: typeof toLabel !== "undefined" ? toLabel : "To"
    property string browseText: typeof browseLabel !== "undefined" ? browseLabel : "Browse"
    property string fileName: defaults.file !== undefined ? defaults.file : ""
    property string format: defaults.format !== undefined ? defaults.format : "postscript"
    property string range: defaults.range !== undefined ? defaults.range : "all"
    property string firstPage: defaults.first !== undefined ? defaults.first : "1"
    property string lastPage: defaults.last !== undefined ? defaults.last : "1"
    property int pageCount: {
        var count = Number(defaults.last);
        return count > 0 && Math.floor(count) === count ? count : 1;
    }
    property string errorText: ""
    property bool browsing: false

    function extensionForFormat(value) {
        return value === "pdf" ? "pdf" : "ps";
    }

    function fileWithExtension(path, extension) {
        var separator = Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\"));
        var dot = path.lastIndexOf(".");
        return dot > separator ? path.slice(0, dot + 1) + extension : path + "." + extension;
    }

    function submit() {
        var first = Number(firstPage);
        var last = Number(lastPage);
        if (fileName.trim().length === 0) {
            errorText = fileText;
            return;
        }
        if (range === "range" && (Math.floor(first) !== first || Math.floor(last) !== last || first < 1 || last < first || last > pageCount)) {
            errorText = pageRangeText;
            return;
        }
        closeBridge.submit({
            "file": fileWithExtension(fileName.trim(), extensionForFormat(format)),
            "format": format,
            "range": range,
            "first": firstPage,
            "last": lastPage
        });
    }

    onCancel: () => {
        if (!root.browsing)
            closeBridge.cancel();
    }

    content: Column {
        spacing: Theme.gapM

        Text {
            text: root.titleText
            color: Theme.fg
            font.pixelSize: Theme.fontBtn
            font.weight: Font.DemiBold
        }

        Row {
            width: parent.width
            height: Theme.rowH
            spacing: Theme.gapS

            Text {
                width: 72 * Theme.scaleFactor
                anchors.verticalCenter: parent.verticalCenter
                text: root.fileText
                color: Theme.fg
                font.pixelSize: Theme.fontBody
            }
            Rectangle {
                width: parent.width - 72 * Theme.scaleFactor - browseButton.width - 2 * Theme.gapS
                height: Theme.rowH
                radius: Theme.radius
                color: Theme.fieldBg
                border.width: Theme.borderW
                border.color: fileInput.activeFocus ? Theme.accent : Theme.borderClr

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: parent.border.width
                    radius: Math.max(0, parent.radius - parent.border.width)
                    clip: true
                    color: parent.color

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.pad
                        anchors.rightMargin: Theme.pad
                        verticalAlignment: Text.AlignVCenter
                        text: root.fileName
                        color: Theme.fg
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideMiddle
                        visible: !fileInput.activeFocus
                    }
                    TextInput {
                        id: fileInput
                        anchors.fill: parent
                        anchors.leftMargin: Theme.pad
                        anchors.rightMargin: Theme.pad
                        verticalAlignment: Text.AlignVCenter
                        color: Theme.fg
                        font.pixelSize: Theme.fontBody
                        selectByMouse: true
                        visible: fileInput.activeFocus
                        text: root.fileName
                        onTextEdited: root.fileName = text
                    }
                    MouseArea {
                        anchors.fill: parent
                        visible: !fileInput.activeFocus
                        cursorShape: Qt.IBeamCursor
                        onClicked: {
                            fileInput.forceActiveFocus();
                            fileInput.cursorPosition = fileInput.length;
                        }
                    }
                }
            }
            MiniButton {
                id: browseButton
                size: "normal"
                text: root.browseText
                onClicked: {
                    root.browsing = true;
                    var picked = closeBridge.chooseSaveFile(root.fileName, root.format, root.titleText);
                    root.browsing = false;
                    if (picked.length > 0)
                        root.fileName = root.fileWithExtension(picked, root.extensionForFormat(root.format));
                }
            }
        }

        EnumCombo {
            width: parent.width
            label: root.formatText
            options: ["pdf", "postscript"]
            optionsTr: ["PDF", "PostScript"]
            value: root.format
            onChanged: function(value) {
                root.format = value;
                if (root.fileName.length > 0)
                    root.fileName = root.fileWithExtension(root.fileName, root.extensionForFormat(value));
            }
        }

        Row {
            width: parent.width
            height: Theme.rowH
            spacing: Theme.gapS

            Text {
                width: 72 * Theme.scaleFactor
                anchors.verticalCenter: parent.verticalCenter
                text: root.pagesText
                color: Theme.fg
                font.pixelSize: Theme.fontBody
            }
            Repeater {
                model: [
                    { "key": "all", "label": root.allPagesText },
                    { "key": "range", "label": root.pageRangeText }
                ]
                delegate: Rectangle {
                    width: Math.max(104 * Theme.scaleFactor, textItem.implicitWidth + 2 * Theme.pad)
                    height: Theme.rowH
                    radius: Theme.radius
                    color: root.range === modelData.key ? Theme.selectBg : Theme.fieldBg
                    border.width: Theme.borderW
                    border.color: root.range === modelData.key ? Theme.selectBorder : Theme.borderClr

                    Text {
                        id: textItem
                        anchors.centerIn: parent
                        text: modelData.label
                        color: root.range === modelData.key ? Theme.selectFg : Theme.fg
                        font.pixelSize: Theme.fontBody
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.range = modelData.key
                    }
                }
            }
        }

        Row {
            visible: root.range === "range"
            width: parent.width
            height: visible ? Theme.rowH : 0
            spacing: Theme.gapS

            Item { width: 72 * Theme.scaleFactor; height: 1 }
            Text {
                width: 40 * Theme.scaleFactor
                anchors.verticalCenter: parent.verticalCenter
                text: root.fromText
                color: Theme.muted
                font.pixelSize: Theme.fontBody
            }
            Rectangle {
                width: 82 * Theme.scaleFactor
                height: Theme.rowH
                radius: Theme.radius
                color: Theme.fieldBg
                border.width: Theme.borderW
                border.color: firstInput.activeFocus ? Theme.accent : Theme.borderClr
                TextInput {
                    id: firstInput
                    anchors.fill: parent
                    anchors.margins: Theme.pad
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.fg
                    font.pixelSize: Theme.fontBody
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1 }
                    text: root.firstPage
                    onTextEdited: root.firstPage = text
                }
            }
            Text {
                width: 24 * Theme.scaleFactor
                anchors.verticalCenter: parent.verticalCenter
                text: root.toText
                color: Theme.muted
                font.pixelSize: Theme.fontBody
            }
            Rectangle {
                width: 82 * Theme.scaleFactor
                height: Theme.rowH
                radius: Theme.radius
                color: Theme.fieldBg
                border.width: Theme.borderW
                border.color: lastInput.activeFocus ? Theme.accent : Theme.borderClr
                TextInput {
                    id: lastInput
                    anchors.fill: parent
                    anchors.margins: Theme.pad
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.fg
                    font.pixelSize: Theme.fontBody
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1 }
                    text: root.lastPage
                    onTextEdited: root.lastPage = text
                }
            }
        }

        Text {
            width: parent.width
            height: errorText.length > 0 ? implicitHeight : 0
            visible: errorText.length > 0
            text: errorText
            color: Theme.muted
            font.pixelSize: Theme.fontMini
            elide: Text.ElideRight
        }

        Item { width: 1; height: Theme.padS }

        DialogButtons {
            anchors.horizontalCenter: parent.horizontalCenter
            buttonLabels: root.buttonLabels
            onClicked: function(index) {
                if (index === 0)
                    root.submit();
                else
                    closeBridge.cancel();
            }
        }
    }
}
