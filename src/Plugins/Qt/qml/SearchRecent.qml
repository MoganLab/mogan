// SearchRecent.qml — 「搜索最近打开的文档」QML 对话框。
// DialogShell + InputField + DialogButtons。一次性提交：OK / 回车把搜索词交给
// cpp_search_recent_dialog，scheme 再调 docgrep-in-recent 对最近文档内容 grep。
// Cancel / Esc 不搜索。本弹窗不按文件名模糊打开。
//
// context property：searchLabel、searchValue、dialogButtons，以及共用
// closeBridge / dpScale / isDark。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 460
    implicitHeight: implicitMargins * 2 + rowH + 12 * Theme.scaleFactor + 64 * Theme.scaleFactor
    implicitMargins: 24 * Theme.scaleFactor

    property string fieldLabel: typeof searchLabel !== "undefined" ? searchLabel : ""
    property string query: typeof searchValue !== "undefined" ? searchValue : ""
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["OK", "Cancel"]
    property real rowH: Theme.rowH

    function submitQuery() {
        closeBridge.submit({ "what": root.query });
    }

    content: Column {
        width: parent ? parent.width : 0
        clip: true
        spacing: 12 * Theme.scaleFactor

        InputField {
            width: parent.width
            label: root.fieldLabel
            value: root.query
            placeholder: ""
            onChanged: function (v) {
                root.query = v;
            }
            onAccepted: root.submitQuery()
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
                    root.submitQuery();
                else
                    closeBridge.cancel();
            }
        }
    }
}
