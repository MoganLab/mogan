// Bibliography.qml — 「插入/修改参考文献」QML 对话框。
// DialogShell + 文件选择行 + (样式 + 缓冲区更新并排) + PreviewPane + DialogButtons。
// 一次性提交：点击 Insert/Modify 提交选中的文件、样式与更新选项；
// Cancel / Esc 放弃。
//
// context property（C++ 注入）：
//   dialogTitle    —— 弹窗标题（已翻译：「插入参考文献」/「修改参考文献」）。
//   dialogPrompt   —— 提示说明（已翻译：「在当前文件插入参考文献」/「在当前文件修改参考文献」）。
//   dialogButtons  —— 按钮文案（已翻译：["插入", "取消"] / ["修改", "取消"]）。
//   fileLabel      —— 「文件:」标签。
//   browseLabel    —— 「浏览」按钮文案。
//   updateLabel    —— 「更新缓冲区:」标签。
//   styleLabel     —— 「样式:」标签。
//   initialFile    —— 初始文件路径。
//   initialStyle   —— 初始样式（默认 "tm-plain"）。
//   initialUpdate  —— 初始是否更新缓冲区（默认 true）。
//   styleOptions   —— 样式列表（"tm-plain", "tm-alpha", ...）。
//   bibBridge      —— BibliographyDialogBridge，提供 browse、toRelativePath、requestPreview。
//   closeBridge    —— QmlDialogBridge。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 760
    implicitHeight: 550

    property string heading: typeof dialogTitle !== "undefined" ? dialogTitle : "Insert bibliography"
    property string prompt: typeof dialogPrompt !== "undefined" ? dialogPrompt : ""
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : ["Insert", "Cancel"]
    property var styles: typeof styleOptions !== "undefined" ? styleOptions : ["tm-plain"]

    property string file: typeof initialFile !== "undefined" ? initialFile : ""
    property string style: typeof initialStyle !== "undefined" ? initialStyle : "tm-plain"
    property bool updateBuffer: typeof initialUpdate !== "undefined" ? initialUpdate : true

    property string previewDataUrl: ""
    property string previewStatus: "empty"
    property string fileHint: ""

    function updatePreview() {
        if (typeof bibBridge !== "undefined" && bibBridge) {
            var res = bibBridge.requestPreview(root.file, root.style);
            if (res) {
                root.previewStatus = res.status || "empty";
                root.previewDataUrl = res.preview || "";
                root.fileHint = res.hint || "";
            }
        }
    }

    // 路径键入逐字符触发，预览须防抖：每次 requestPreview 都同步走
    // 读文件 + parse-bib + 排版光栅化，不防抖会卡住键入
    Timer {
        id: previewDebounce
        interval: 350
        onTriggered: root.updatePreview()
    }

    function submit() {
        if (typeof closeBridge !== "undefined" && closeBridge) {
            closeBridge.submit({
                "file": root.file,
                "style": root.style,
                "update": root.updateBuffer ? "true" : "false"
            });
        }
    }

    // 故意不挂 onActivate：Browse 弹的原生文件对话框里按 Enter
    // 确认时，焦点切回瞬间按键会泄漏触发 submit；故确认只认「插入」/「修改」主按钮。
    onCancel: () => closeBridge.cancel()

    Component.onCompleted: {
        updatePreview();
    }

    content: Column {
        id: mainCol
        width: parent ? parent.width : 0
        spacing: Theme.gapM

        // 标题与说明
        Column {
            width: mainCol.width
            spacing: Theme.padS

            Text {
                width: parent.width
                text: root.heading
                color: Theme.fg
                font.pixelSize: Theme.fontBody + 3 * Theme.scaleFactor
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                visible: root.prompt.length > 0
                width: parent.width
                text: root.prompt
                color: Theme.muted
                font.pixelSize: Theme.fontSmall
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // 1. 文件路径输入行：InputField 原子（标签 + 输入框 + 浏览按钮）
        InputField {
            id: fileField
            width: mainCol.width
            labelWidth: 45 * Theme.scaleFactor
            label: typeof fileLabel !== "undefined" ? fileLabel : "File:"
            actionLabel: typeof browseLabel !== "undefined" ? browseLabel : "Browse"
            value: root.file
            onChanged: function (v) {
                root.file = v;
                previewDebounce.restart();
            }
            onActionClicked: {
                if (typeof bibBridge !== "undefined" && bibBridge) {
                    var chosen = bibBridge.browse(root.file);
                    if (chosen && chosen.length > 0) {
                        chosen = bibBridge.toRelativePath(chosen);
                        root.file = chosen;
                        // 用户键入已打断 value 绑定，显式回赋刷新显示
                        fileField.value = chosen;
                        root.updatePreview();
                    }
                }
            }
        }

        // 文件非法提示（若 bib 路径无效/文件不存在/损坏，直接在此给出明确提示）
        Text {
            visible: root.fileHint.length > 0
            width: mainCol.width
            text: root.fileHint
            color: "#e06c75"
            font.pixelSize: Theme.fontSmall
            font.bold: true
            anchors.leftMargin: 45 * Theme.scaleFactor + Theme.gapM
        }

        // 2. 样式与选项并排整行：样式选择 + 更新缓冲区
        Row {
            width: mainCol.width
            height: Theme.rowH
            spacing: Theme.twoColGap

            // 样式选择（标签宽 45，下拉框充裕展开）
            Item {
                id: styleCell
                width: 280 * Theme.scaleFactor
                height: Theme.rowH

                EnumCombo {
                    anchors.fill: parent
                    labelWidth: 45 * Theme.scaleFactor
                    label: typeof styleLabel !== "undefined" ? styleLabel : "Style:"
                    options: root.styles
                    value: root.style
                    onChanged: function (v) {
                        root.style = v;
                        root.updatePreview();
                    }
                }
            }

            // 更新缓冲区
            Item {
                width: 200 * Theme.scaleFactor
                height: Theme.rowH

                Toggle {
                    anchors.fill: parent
                    labelWidth: 140 * Theme.scaleFactor
                    label: typeof updateLabel !== "undefined" ? updateLabel : "Update buffer:"
                    value: root.updateBuffer
                    onToggled: function (val) {
                        root.updateBuffer = val;
                    }
                }
            }
        }

        // 3. 预览区：卡片边框容器
        Rectangle {
            width: mainCol.width
            height: 200 * Theme.scaleFactor
            color: Theme.fieldBg
            radius: Theme.radius
            border.width: Theme.borderW
            border.color: Theme.borderClr
            clip: true

            // 未选文件时的原生高清矢量占位提示（清晰锐利、无缩放模糊）
            Text {
                visible: root.previewStatus === "empty"
                anchors.centerIn: parent
                text: typeof emptyHint !== "undefined" ? emptyHint : "Preview the bibliography format here"
                color: Theme.muted
                font.pixelSize: Theme.fontBody
            }

            // 文件不存在或非法时的原生矢量提示
            Text {
                visible: root.previewStatus === "not_found" || root.previewStatus === "invalid"
                anchors.centerIn: parent
                text: root.fileHint
                color: "#e06c75"
                font.pixelSize: Theme.fontBody
                font.bold: true
            }

            // 合法文献排版预览图
            PreviewPane {
                visible: root.previewStatus === "valid" && root.previewDataUrl.length > 0
                anchors.fill: parent
                anchors.margins: Theme.padS
                imageSource: root.previewDataUrl
            }
        }

        // 4. 底部操作按钮（未选/非法时主按钮置灰禁用）
        DialogButtons {
            anchors.right: parent.right
            buttonLabels: root.buttonLabels
            primaryEnabled: root.previewStatus === "valid"
            onClicked: function (index) {
                if (index === 0)
                    root.submit();
                else
                    closeBridge.cancel();
            }
        }
    }
}
