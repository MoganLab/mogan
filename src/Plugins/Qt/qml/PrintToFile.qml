// PrintToFile.qml — 「打印页面选择到文件」QML 对话框。
// DialogShell 外壳 + InputField 原子（路径/页码）+ DialogButtons。一次性提交：
// 用户点 OK 时 closeBridge.submit(values) 整包带回 name/first/last，C++ 侧
// cpp_print_to_file_dialog 解读后交给 scheme 走 print-pages-to-file；Cancel 放弃。
//
// context property（C++ 注入）：
//   formFields   —— 字段表 [{type,label,key,value}, ...]，type 为 "path"（有
//                   Browse 按钮）或 "number"（仅数字）。label/value 在 scm 侧
//                   已翻译/取好（value 为初值）。
//   dialogButtons—— OK/Cancel 文案，经 qt_translate，跟随界面语言。
//   browseLabel  —— Path 字段的「Browse」按钮文案（scm 翻译）。
//   printBridge  —— C++ bridge，browse() 弹原生保存文件对话框取路径。
//   再含共用 closeBridge / dpScale / isDark。
//
// OK：closeBridge.submit({name:..., first:..., last:...})；Cancel：closeBridge.cancel()。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 460
    // 动态高度：内边距 + 字段行（行高 + 行间距）+ 按钮区（同 FormDialog 同源）。
    implicitHeight: implicitMargins * 2 + fields.length * (rowH + 12 * Theme.scaleFactor) + 64 * Theme.scaleFactor
    implicitMargins: 24 * Theme.scaleFactor

    property var fields: typeof formFields !== "undefined" ? formFields : []
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["OK", "Cancel"]
    property string browseButtonLabel: typeof browseLabel !== "undefined" ? browseLabel : "Browse"
    property real rowH: Theme.rowH

    // 字段运行时值：Repeater 的 modelData 只读，故另起对象存当前值，OK 时整包提交。
    // 键入/浏览经 onChanged/onActionClicked 改此对象再回赋，触发 InputField 的
    // value binding 刷新显示。
    property var values: {
        var v = {};
        for (var i = 0; i < fields.length; i++)
            v[fields[i].key] = fields[i].value;
        return v;
    }

    content: Column {
        width: parent ? parent.width : 0
        clip: true
        spacing: 12 * Theme.scaleFactor

        Repeater {
            model: root.fields
            delegate: InputField {
                width: parent.width
                label: modelData.label
                numeric: modelData.type === "number"
                actionLabel: modelData.type === "path" ? root.browseButtonLabel : ""
                value: root.values[modelData.key] !== undefined ? root.values[modelData.key] : ""
                onChanged: function (v) {
                    var cur = root.values;
                    cur[modelData.key] = v;
                    root.values = cur;
                }
                onActionClicked: function () {
                    if (modelData.type !== "path")
                        return;
                    var p = printBridge.browse(root.values[modelData.key]);
                    if (p) {
                        var cur = root.values;
                        cur[modelData.key] = p;
                        root.values = cur;
                    }
                }
                onAccepted: closeBridge.submit(root.values)
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
