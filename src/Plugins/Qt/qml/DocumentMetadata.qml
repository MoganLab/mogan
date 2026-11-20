// DocumentMetadata.qml — 「文档 → 元数据...」QML 对话框。
// DialogShell + InputField（标题/作者/主题）+ DialogButtons（重置/确定/取消）。
// 一次性提交：本地暂存表单值，点击「重置」恢复为默认值并标记 resetPending，
// 点击「确定」经 closeBridge.submit(...) 交回 Scheme；「取消」/ Esc 放弃。
//
// context property（C++ 注入）：
//   dialogTitle   —— 已翻译的弹窗标题（「文档元数据」）
//   formFields    —— [{key, label, value, defaultValue}, ...]
//   dialogButtons —— [Reset, OK, Cancel] 已翻译按钮文案
//   以及共用 closeBridge / dpScale / isDark。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 460
    implicitMargins: Theme.margin
    implicitHeight: Theme.margin * 2 + Theme.titleH + fields.length * (Theme.rowH + Theme.gapM) + Theme.btnH + Theme.gapM * 2 + Theme.gapS

    property string titleText: typeof dialogTitle !== "undefined" ? dialogTitle : ""
    property var fields: typeof formFields !== "undefined" ? formFields : []
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["Reset", "OK", "Cancel"]
    property bool resetPending: false

    property var values: {
        var v = {};
        for (var i = 0; i < fields.length; i++)
            v[fields[i].key] = fields[i].value !== undefined ? fields[i].value : "";
        return v;
    }

    function resetToDefaults() {
        var v = {};
        for (var i = 0; i < root.fields.length; i++) {
            var f = root.fields[i];
            v[f.key] = f.defaultValue !== undefined ? f.defaultValue : "";
        }
        root.values = v;
        root.resetPending = true;
    }

    function submitMetadata() {
        var payload = {};
        for (var i = 0; i < root.fields.length; i++) {
            var k = root.fields[i].key;
            payload[k] = root.values[k] !== undefined ? root.values[k] : "";
        }
        payload["reset"] = root.resetPending ? "true" : "false";
        closeBridge.submit(payload);
    }

    onActivate: () => root.submitMetadata()

    content: Column {
        width: parent ? parent.width : 0
        clip: true
        spacing: Theme.gapM

        Text {
            width: parent.width
            height: Theme.titleH
            text: root.titleText
            color: Theme.fg
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 18 * Theme.scaleFactor
            font.weight: Font.Bold
            font.letterSpacing: 0.2 * Theme.scaleFactor
            elide: Text.ElideRight
        }

        Repeater {
            model: root.fields
            delegate: InputField {
                width: parent.width
                label: modelData.label
                value: root.values[modelData.key] !== undefined ? root.values[modelData.key] : ""
                Component.onCompleted: {
                    if (index === 0)
                        forceFocus();
                }
                onChanged: function (v) {
                    var cur = root.values;
                    cur[modelData.key] = v;
                    root.values = cur;
                    root.resetPending = false;
                }
                onAccepted: root.submitMetadata()
            }
        }

        Item {
            width: 1
            height: Theme.gapS
        }

        DialogButtons {
            anchors.horizontalCenter: parent.horizontalCenter
            buttonLabels: root.buttonLabels
            primaryIndex: 1
            onClicked: function (index) {
                if (index === 0)
                    root.resetToDefaults();
                else if (index === 1)
                    root.submitMetadata();
                else
                    closeBridge.cancel();
            }
        }
    }
}
