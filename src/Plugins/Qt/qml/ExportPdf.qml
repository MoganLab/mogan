// ExportPdf.qml — 「文件 → 导出为 PDF」，交互对齐 WPS「输出为 PDF」：
// 文件名与位置分开；浏览只选文件夹（不是另存为），避免系统保存框把未写出的
// PDF 记进「最近文件」。页码 + 是否嵌入 tmu。导出提交
// folder/filename/first/last/embed。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 520
    implicitHeight: implicitMargins * 2 + titleH + 12 * Theme.scaleFactor
                    + 4 * (rowH + 12 * Theme.scaleFactor) + rowH + 72 * Theme.scaleFactor
    implicitMargins: 24 * Theme.scaleFactor
    onActivate: function () { closeBridge.submit(root.payload()) }

    property var fields: typeof formFields !== "undefined" ? formFields : []
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["Export", "Cancel"]
    property string browseButtonLabel: typeof browseLabel !== "undefined" ? browseLabel : "Browse"
    property string heading: typeof dialogTitle !== "undefined" ? dialogTitle : "Export as PDF"
    property string pagesText: typeof pagesLabel !== "undefined" ? pagesLabel : "Pages:"
    property real rowH: Theme.rowH
    property real titleH: Theme.titleH

    property var values: {
        var v = {};
        for (var i = 0; i < fields.length; i++)
            v[fields[i].key] = fields[i].value;
        return v;
    }

    function setv(k, x) {
        var cur = root.values;
        cur[k] = x;
        root.values = cur;
    }

    function val(k) {
        return root.values[k] !== undefined ? root.values[k] : "";
    }

    function destPath() {
        var f = String(root.val("folder"));
        var n = String(root.val("filename"));
        if (!n)
            return "";
        if (n.length < 4 || n.toLowerCase().slice(-4) !== ".pdf")
            n = n + ".pdf";
        if (!f)
            return n;
        var last = f.charAt(f.length - 1);
        if (last === "/" || last === "\\")
            return f + n;
        var sep = f.indexOf("\\") >= 0 ? "\\" : "/";
        return f + sep + n;
    }

    function payload() {
        var v = {};
        for (var i = 0; i < fields.length; i++)
            v[fields[i].key] = root.val(fields[i].key);
        v.name = root.destPath();
        return v;
    }

    function fieldLabel(key, fallback) {
        for (var i = 0; i < root.fields.length; i++)
            if (root.fields[i].key === key)
                return root.fields[i].label;
        return fallback;
    }

    content: Column {
        width: parent ? parent.width : 0
        clip: true
        spacing: 12 * Theme.scaleFactor

        Text {
            width: parent.width
            height: root.titleH
            text: root.heading
            color: Theme.fg
            font.pixelSize: Theme.fontBtn
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        InputField {
            width: parent.width
            label: root.fieldLabel("filename", "File name:")
            value: root.val("filename")
            onChanged: function (v) { root.setv("filename", v) }
            onAccepted: closeBridge.submit(root.payload())
        }

        InputField {
            width: parent.width
            label: root.fieldLabel("folder", "Location:")
            actionLabel: root.browseButtonLabel
            value: root.val("folder")
            onChanged: function (v) { root.setv("folder", v) }
            onActionClicked: function () {
                var p = printBridge.browseFolder(root.val("folder"));
                if (p)
                    root.setv("folder", p);
            }
            onAccepted: closeBridge.submit(root.payload())
        }

        Row {
            width: parent.width
            height: root.rowH
            spacing: Theme.gapM

            Text {
                width: parent.width * 0.35
                height: parent.height
                text: root.pagesText
                color: Theme.fg
                font.pixelSize: Theme.fontBody
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            Rectangle {
                width: (parent.width * 0.65 - parent.spacing * 2 - 24 * Theme.scaleFactor) / 2
                height: parent.height
                radius: Theme.radius
                color: firstTxt.activeFocus ? Theme.fieldBgHover : Theme.fieldBg
                border.width: Theme.borderW
                border.color: firstTxt.activeFocus ? Theme.accent : Theme.borderClr

                TextInput {
                    id: firstTxt
                    anchors.fill: parent
                    anchors.margins: Theme.comboPad
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.fg
                    font.pixelSize: Theme.fontBody
                    selectByMouse: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    text: root.val("first")
                    onTextChanged: root.setv("first", text)
                    onAccepted: closeBridge.submit(root.payload())
                }
            }

            Text {
                width: 24 * Theme.scaleFactor
                height: parent.height
                text: "–"
                color: Theme.muted
                font.pixelSize: Theme.fontBody
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                width: (parent.width * 0.65 - parent.spacing * 2 - 24 * Theme.scaleFactor) / 2
                height: parent.height
                radius: Theme.radius
                color: lastTxt.activeFocus ? Theme.fieldBgHover : Theme.fieldBg
                border.width: Theme.borderW
                border.color: lastTxt.activeFocus ? Theme.accent : Theme.borderClr

                TextInput {
                    id: lastTxt
                    anchors.fill: parent
                    anchors.margins: Theme.comboPad
                    verticalAlignment: Text.AlignVCenter
                    color: Theme.fg
                    font.pixelSize: Theme.fontBody
                    selectByMouse: true
                    inputMethodHints: Qt.ImhDigitsOnly
                    text: root.val("last")
                    onTextChanged: root.setv("last", text)
                    onAccepted: closeBridge.submit(root.payload())
                }
            }
        }

        Toggle {
            width: parent.width
            height: root.rowH
            label: root.fieldLabel("embed", "Embed TMU source")
            hint: ""
            value: root.val("embed") === "true"
            onToggled: function (on) { root.setv("embed", on ? "true" : "false") }
        }

        Item {
            width: 1
            height: 8 * Theme.scaleFactor
        }

        DialogButtons {
            anchors.horizontalCenter: parent.horizontalCenter
            buttonLabels: root.buttonLabels
            onClicked: function (index) {
                if (index === 0)
                    closeBridge.submit(root.payload());
                else
                    closeBridge.cancel();
            }
        }
    }
}
