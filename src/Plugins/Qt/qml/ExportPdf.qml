// ExportPdf.qml — 「导出为PDF」对话框。本次只含一个选项：是否把源文档作为
// 附件嵌入 PDF（Toggle 原子）；后续选项（文件名/位置/页码等）在此追加。
// 一次性提交：确认时 closeBridge.submit(values) 整包带回（value 统一 string，
// 布尔用 "true"/"false"），C++ 侧 cpp_export_pdf_dialog 解读后交 scheme 决定
// 走可编辑 PDF 导出还是普通导出；取消放弃。
//
// context property（C++ 注入）：
//   formFields    —— 字段表 [{type:"toggle", label, key, value}, ...]，type 暂只
//                   支持 "toggle"。label/value 在 scm 侧已翻译/取好（value 为
//                   初值，"true"/"false"）。
//   dialogButtons —— 确认/取消文案，经 qt_translate，跟随界面语言。
//   dialogTitle   —— 弹窗标题（qt_translate 翻译）。
//   再含共用 closeBridge / dpScale / isDark。
//
// 确认（按钮 / Enter）：closeBridge.submit(values)；取消（按钮 / ESC）：closeBridge.cancel()。

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
    onActivate: function () { closeBridge.submit(root.values) }

    property var fields: typeof formFields !== "undefined" ? formFields : []
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["Export", "Cancel"]
    property string heading: typeof dialogTitle !== "undefined" ? dialogTitle : "Export as PDF"
    property real rowH: Theme.rowH
    property real titleH: Theme.titleH

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
            model: root.fields
            delegate: Toggle {
                width: parent.width
                label: modelData.label
                value: root.values[modelData.key] === "true"
                onToggled: function (on) {
                    root.setv(modelData.key, on ? "true" : "false")
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
