// ExportPdf.qml — 「导出为PDF」对话框。选项：是否把源文档作为附件嵌入 PDF
// （Toggle 原子）+ 导出目的地（完整路径含文件名，只读展示 + Browse 换路径）。
// 一次性提交：确认时 closeBridge.submit(values) 整包带回（value 统一 string，
// 布尔用 "true"/"false"），C++ 侧 cpp_export_pdf_dialog 解读后交 scheme 决定
// 走可编辑 PDF 导出还是普通导出；取消放弃。
//
// context property（C++ 注入）：
//   formFields    —— 字段表 [{type, label, key, value}, ...]，type 为 "toggle"
//                   或 "path"。label/value 在 scm 侧已翻译/取好（value 为初值，
//                   "true"/"false" 或完整路径）。
//   dialogButtons —— 确认/取消文案，经 qt_translate，跟随界面语言。
//   dialogTitle   —— 弹窗标题（qt_translate 翻译）。
//   browseLabel   —— 目的地的「Browse」按钮文案（qt_translate 翻译）。
//   browseBridge  —— C++ bridge，browse(current) 弹原生保存对话框取完整路径。
//   再含共用 closeBridge / dpScale / isDark。
//
// 确认（仅「导出」按钮，不挂 Enter，见下方 onActivate 注释）：closeBridge.submit(values)；
// 取消（按钮 / ESC）：closeBridge.cancel()。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 460
    // 动态高度：内边距 + 标题行（行高 + 行间距）+ 选项行 + 按钮区（同 PrintToFile 同源）。
    implicitHeight: implicitMargins * 2 + titleH + 12 * Theme.scaleFactor
                    + fields.length * (rowH + 12 * Theme.scaleFactor)
                    + 8 * Theme.scaleFactor + 72 * Theme.scaleFactor
    implicitMargins: 24 * Theme.scaleFactor
    // 故意不挂 onActivate（Enter 确认）：Browse 弹的原生保存对话框里按 Enter
    // 确认时，焦点切回弹窗的瞬间按键会泄漏到 DialogShell 触发 submit，导出被
    // 意外提前触发；去掉 Enter 链路后确认只认「导出」按钮（PrintToFile 同）。

    property var fields: typeof formFields !== "undefined" ? formFields : []
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["Export", "Cancel"]
    property string heading: typeof dialogTitle !== "undefined" ? dialogTitle : "Export as PDF"
    property string browseButtonLabel: typeof browseLabel !== "undefined" ? browseLabel : "Browse"
    property real rowH: Theme.rowH
    property real titleH: Theme.titleH

    // path 型字段单列渲染（只读路径行）；Repeater 只出其余字段（toggle）。
    property var toggleFields: {
        var t = [];
        for (var i = 0; i < fields.length; i++)
            if (fields[i].type !== "path") t.push(fields[i]);
        return t;
    }
    property var pathField: {
        for (var i = 0; i < fields.length; i++)
            if (fields[i].type === "path") return fields[i];
        return null;
    }
    property string pathKey: pathField ? pathField.key : ""
    property string pathValue: root.values[pathKey] !== undefined ? root.values[pathKey] : ""

    // 字段运行时值：Repeater 的 modelData 只读，故另起对象存当前值，确认时整包提交。
    // 切换经 onToggled 改此对象再回赋，触发 delegate 的 value binding 刷新显示。
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

        Repeater {
            model: root.toggleFields
            delegate: Toggle {
                width: parent.width
                label: modelData.label
                value: root.values[modelData.key] === "true"
                onToggled: function (on) {
                    root.setv(modelData.key, on ? "true" : "false")
                }
            }
        }

        // 目的地行：label + 只读路径展示 + Browse。label 取文案自然宽度（固定
        // 比例会挤占路径区），余宽全给路径；路径过长时中间省略（保住开头目录
        // 与结尾文件名），不长则原样展示；换路径只经 Browse。
        Row {
            width: parent.width
            spacing: Theme.gapM
            visible: root.pathField !== null

            Text {
                id: destLabel
                anchors.verticalCenter: parent.verticalCenter
                text: root.pathField ? root.pathField.label : ""
                color: Theme.fg
                font.pixelSize: Theme.fontBody
            }

            Rectangle {
                width: parent.width - destLabel.implicitWidth - browseBtn.width
                       - 2 * parent.spacing
                height: root.rowH
                anchors.verticalCenter: parent.verticalCenter
                radius: Theme.radius
                clip: true
                color: Theme.fieldBg
                border.width: Theme.borderW
                border.color: Theme.borderClr

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.comboPad
                    anchors.rightMargin: Theme.comboPad
                    verticalAlignment: Text.AlignVCenter
                    text: root.pathValue
                    color: Theme.fg
                    font.pixelSize: Theme.fontBody
                    elide: Text.ElideMiddle
                }
            }

            MiniButton {
                id: browseBtn
                size: "normal"
                anchors.verticalCenter: parent.verticalCenter
                text: root.browseButtonLabel
                onClicked: {
                    var p = browseBridge.browse(root.pathValue);
                    if (p) root.setv(root.pathKey, p);
                }
            }
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
                    closeBridge.submit(root.values);
                else
                    closeBridge.cancel();
            }
        }
    }
}
