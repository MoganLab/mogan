// FormDialog.qml — 通用 form 弹窗。
// DialogShell + EnumCombo + DialogButtons 拼装，仅剩字段 Repeater + values 收集 +
// submit。下拉浮层由 DialogShell 的共享 overlay 提供（不在此文件）。
//
// context property（C++ 注入）：formFields、dialogButtons、dpScale、isDark、
// closeBridge。dialogButtons 经 qt_translate，跟随当前界面语言。
// OK：closeBridge.submit({key: value, ...})；Cancel：closeBridge.cancel()。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 420
    // 动态高度：内边距 + 字段行（行高 + 行间距）+ 按钮区。
    implicitHeight: implicitMargins * 2 + fields.length * (rowH + 12 * Theme.scaleFactor) + 64 * Theme.scaleFactor
    implicitMargins: 24 * Theme.scaleFactor

    property var fields: typeof formFields !== "undefined" ? formFields : []
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["OK", "Cancel"]
    property real rowH: Theme.rowH

    // 字段运行时值：Repeater 的 modelData 只读，改不了，故另起一个对象存当前值，
    // OK 时整包提交。onChanged 里改这个对象再回赋，触发 binding 刷新 combo 显示。
    property var values: {
        var v = {};
        for (var i = 0; i < fields.length; i++)
            v[fields[i].key] = fields[i].value;
        return v;
    }

    content: Column {
        // 窗口高度按字段数动态算（C++ logicH 与本文件 implicitHeight 同源），正常不
        // 溢出；clip 作防御性兜底，字段异常多时不撑破正文区视觉。
        clip: true
        spacing: 12 * Theme.scaleFactor

        Repeater {
            model: root.fields
            delegate: EnumCombo {
                width: parent.width
                label: modelData.label
                options: modelData.options !== undefined ? modelData.options : []
                value: root.values[modelData.key] !== undefined ? root.values[modelData.key] : ""
                onChanged: function (v) {
                    var cur = root.values;
                    cur[modelData.key] = v;
                    root.values = cur;
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
