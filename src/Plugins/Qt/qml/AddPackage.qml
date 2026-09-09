// AddPackage.qml — 「增加宏包」QML 对话框。
// DialogShell + InputField + DialogButtons。一次性提交：OK / 回车把宏包名交给
// cpp_add_package_dialog，scheme 再调 add-style-package 追加宏包。
// Cancel / Esc 不添加。
//
// context property：packageLabel、packageName、dialogButtons，以及共用
// closeBridge / dpScale / isDark。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 360
    implicitHeight: implicitMargins * 2 + rowH + 12 * Theme.scaleFactor + 64 * Theme.scaleFactor
    implicitMargins: 24 * Theme.scaleFactor

    property string fieldLabel: typeof packageLabel !== "undefined" ? packageLabel : ""
    property string currentPackage: typeof packageName !== "undefined" ? packageName : ""
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["OK", "Cancel"]
    property real rowH: Theme.rowH

    property bool submitted: false
    function submitPackage() {
        if (root.submitted) return;
        root.submitted = true;
        closeBridge.submit({ "package": root.currentPackage.trim() });
    }

    onActivate: () => root.submitPackage()

    content: Column {
        width: parent ? parent.width : 0
        spacing: 12 * Theme.scaleFactor

        InputField {
            id: pkgInput
            width: parent.width
            label: root.fieldLabel
            labelWidth: pkgInput.labelImplicitWidth
            value: root.currentPackage
            placeholder: ""
            onChanged: function (v) {
                root.currentPackage = v;
            }
            onAccepted: root.submitPackage()
            Component.onCompleted: pkgInput.forceFocus()
        }

        Item {
            width: 1
            height: 8 * Theme.scaleFactor
        }

        DialogButtons {
            anchors.right: parent.right
            buttonLabels: root.buttonLabels
            onClicked: function (index) {
                if (index === 0)
                    root.submitPackage();
                else
                    closeBridge.cancel();
            }
        }
    }
}
