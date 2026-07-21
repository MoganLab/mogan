// Preferences.qml — 首选项弹窗（「编辑 → 首选项」）。
// 5 主 tab（General / Keyboard / Mathematics / Convert / Other），其中 Convert 下
// 再有 6 子 tab（Html / LaTeX / BibTeX / Verbatim / Pdf / Image）。约 40 combo + 35 toggle
// + 若干 info row。设计稿见 ai-docs/qml/qml-dialog.html 的 #panel-preferences。
//
// 本地暂存 + OK 一次性提交（FormDialog 模式——参考 FormDialog.qml / ParagraphFormat.qml）：
//   - 打开时 prefBridge.meta() 一次性拉全部 tab/字段描述符树到 initialValues（打开快照）
//   - 用户改动只改本地 values（条件可见性 / radio 互斥也纯本地——不往 facade 写）
//   - OK 时 changedFields() 算 diff（仅与快照不同的键）→ prefBridge.submit(diff) 一次性应用
//   - Cancel 丢弃（prefBridge.cancel() 只关窗、scheme 侧 no-op）
//
// field-descriptor 协议（assoc-list of pairs，见 ai-docs/qml/README.md 的 Preferences 契约）：
//   每个 field 为 {kind, key, label, value, options?, optionsTr?, editable?, restart?,
//     radioGroup?, visibleWhenKey?, visibleWhenVal?, group?, hint?, column?}
//   kind 分流：combo -> EnumCombo、toggle -> Toggle、info -> label + 只读 Text
//
// prefBridge 契约（PreferencesBridge，无状态透传）：
//   meta() -> QVariantMap{tabs: [{key, label, fields, subTabs?}]}
//   submit(QVariantMap changed) -> QString（"applied" / "restart" / "later" / "cancel"）
//   cancel() / startMove()
//
// 条件可见性 / radio 互斥：纯 QML 本地（values[field.visibleWhenKey] === visibleWhenVal 才可见；
// radio peer 开一个则同组其它置 false）。详见 ai-docs/qml/README.md 的「条件可见性 / radio 互斥」节。

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 560
    implicitHeight: 600
    implicitMargins: 24 * Theme.scaleFactor
    // Enter/Return 默认 OK（本地暂存提交）。
    onActivate: () => root.submit()

    property var meta: typeof prefBridge !== "undefined" ? prefBridge.meta() : ({
            tabs: []
        })
    // OK/Cancel 文案：bridge 注入已翻译的 dialogButtons（同 FormDialog），回退 qsTr。
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : [qsTr("OK"), qsTr("Cancel")]

    // 打开快照：从 meta 一次性建 {key: value}（只含 combo/toggle——info 不入 editable map）。
    // initialValues 打开瞬间固定、values 是可变副本——changedFields 比 initialValues 算 diff。
    property var initialValues: root.buildValues(root.meta)
    property var values: {
        var v = {};
        for (var k in root.initialValues)
            v[k] = root.initialValues[k];
        return v;
    }

    property string activeTab: "general"
    property string activeSubTab: "html"  // Convert tab 的子 tab

    // 从 meta 树建 {key: value}（combo/toggle 的 key/value；info 无 setter、不入 editable map）。
    function buildValues(metaObj) {
        var v = {};
        var tabs = metaObj && metaObj.tabs ? metaObj.tabs : [];
        for (var i = 0; i < tabs.length; i++) {
            var tab = tabs[i];
            collectValues(tab.fields || [], v);
            var subs = tab.subTabs || [];
            for (var j = 0; j < subs.length; j++)
                collectValues(subs[j].fields || [], v);
        }
        return v;
    }
    function collectValues(fields, target) {
        for (var i = 0; i < fields.length; i++) {
            var f = fields[i];
            if (f.kind === "info")
                // info 无 setter、不入 editable map
                continue;
            if (!f.key)
                continue;
            target[f.key] = f.value !== undefined ? f.value : "";
        }
    }

    // 改某字段：更新本地 values（触发显示刷新）。radio 互斥：若该字段有 radioGroup 且新值
    // 是 "on"（开一个），则把同组其它 peer 在本地 values 里置 "off"。
    function setField(key, val) {
        var cur = root.values;
        cur[key] = val;
        // radio 互斥：查当前 field 的 radioGroup，若开了则关同组其它 peer。
        var field = root.findField(key);
        if (field && field.radioGroup && val === "on") {
            var peers = root.fieldsInRadioGroup(field.radioGroup);
            for (var i = 0; i < peers.length; i++)
                if (peers[i].key !== key)
                    cur[peers[i].key] = "off";
        }
        root.values = cur;
    }

    // 算 diff：仅与打开快照不同的键。toggle 的值已是 "on"/"off" 串（wire 格式统一字符串）。
    function changedFields() {
        var d = {};
        for (var k in root.values)
            if (root.values[k] !== root.initialValues[k])
                d[k] = root.values[k];
        return d;
    }

    // 按字段 key 查 meta 树里的 field 描述符（用于 radio peer 查找）。
    function findField(key) {
        var tabs = root.meta && root.meta.tabs ? root.meta.tabs : [];
        for (var i = 0; i < tabs.length; i++) {
            var found = findFieldInList(tabs[i].fields || [], key);
            if (found)
                return found;
            var subs = tabs[i].subTabs || [];
            for (var j = 0; j < subs.length; j++) {
                var f2 = findFieldInList(subs[j].fields || [], key);
                if (f2)
                    return f2;
            }
        }
        return null;
    }
    function findFieldInList(fields, key) {
        for (var i = 0; i < fields.length; i++)
            if (fields[i].key === key)
                return fields[i];
        return null;
    }
    // 返回某 radioGroup 下所有 field 描述符（用于互斥关 peer）。
    function fieldsInRadioGroup(groupName) {
        var out = [];
        function scan(fields) {
            for (var i = 0; i < fields.length; i++)
                if (fields[i].radioGroup === groupName)
                    out.push(fields[i]);
        }
        var tabs = root.meta && root.meta.tabs ? root.meta.tabs : [];
        for (var i = 0; i < tabs.length; i++) {
            scan(tabs[i].fields || []);
            var subs = tabs[i].subTabs || [];
            for (var j = 0; j < subs.length; j++)
                scan(subs[j].fields || []);
        }
        return out;
    }

    function submit() {
        var diff = root.changedFields();
        if (Object.keys(diff).length === 0) {
            // 无改动：直接关窗。
            closeBridge.cancel();
            return;
        }
        prefBridge.submit(diff);  // facade 按「先确认再 apply」三分支处理
        closeBridge.cancel();  // 关窗（submit 始终关闭）
    }

    content: Item {
        id: shell

        // 顶部主 TabBar（General / Keyboard / Mathematics / Convert / Other）。
        TabBar {
            id: mainTabs
            anchors.top: parent.top
            anchors.left: parent.left
            model: root.meta && root.meta.tabs ? root.meta.tabs.map(function (t) {
                return {
                    key: t.key,
                    label: t.label
                };
            }) : []
            activeKey: root.activeTab
            onSelected: function (key) {
                root.activeTab = key;
            }
        }

        // 底部按钮区（OK / Cancel）。
        DialogButtons {
            id: bottomButtons
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            buttonLabels: root.buttonLabels
            primaryIndex: 0  // OK 默认主按钮
            buttonWidth: 90 * Theme.scaleFactor
            onClicked: function (i) {
                if (i === 0)
                    root.submit();
                else
                    closeBridge.cancel();
            }
        }

        // 正文区：main TabBar 与按钮区之间。Convert tab 在顶部正文区再渲染子 TabBar + 子 Flickable。
        Item {
            id: body
            anchors.top: mainTabs.bottom
            anchors.topMargin: 14 * Theme.scaleFactor
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: bottomButtons.top
            anchors.bottomMargin: 14 * Theme.scaleFactor
            clip: true

            // 当前 active tab 对象（从 meta.tabs 按 root.activeTab 查找；property 声明
            // 让 binding 在 activeTab 变化时自动重算）。
            property var activeTabObj: root.meta && root.meta.tabs ? root.meta.tabs.find(function (t) {
                return t.key === root.activeTab;
            }) : null

            // Convert 子 TabBar（仅 activeTab === "convert" 且该 tab 有 subTabs 时显示）。
            TabBar {
                id: subTabs
                anchors.top: parent.top
                anchors.left: parent.left
                visible: root.activeTab === "convert" && !!body.activeTabObj && !!body.activeTabObj.subTabs
                model: visible && body.activeTabObj && body.activeTabObj.subTabs ? body.activeTabObj.subTabs.map(function (s) {
                    return {
                        key: s.key,
                        label: s.label
                    };
                }) : []
                activeKey: root.activeSubTab
                onSelected: function (key) {
                    root.activeSubTab = key;
                }
            }

            // 字段 Flickable：通用渲染。按 activeTab 决定渲染哪个 fields 列表：
            //   非 Convert tab -> activeTabObj.fields
            //   Convert tab -> activeSubTabObj.fields（子 tab 的 fields）
            // 通用 Repeater delegate 按 field.kind 分流 -> EnumCombo / Toggle / info row。
            // 条件可见性 binding、radio 互斥在 setField helper 里处理（见顶部注释）。
            Flickable {
                id: fieldScroll
                anchors.fill: parent
                anchors.topMargin: (root.activeTab === "convert" && body.activeTabObj && body.activeTabObj.subTabs) ? subTabs.height + Theme.padS : 0
                contentWidth: width
                contentHeight: fieldCol.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                // 字段 delegate（单栏 / 双栏左右列共用）：按 field.kind 分流 combo/toggle/info。
                // 三种实例并存、只显示对应 kind（避免 Loader/Component 的复杂度）。
                // 条件可见性在外层 Item 的 visible binding 统一处理——不可见整行 hide、不占位。
                Component {
                    id: fieldDelegate
                    Item {
                        width: parent ? parent.width : 0
                        // 高度按当前 kind 的可见实例取（EnumCombo/Toggle 固定行高 Theme.rowH；
                        // info 行 textRowH；不可见时归零、不占位）。
                        height: visible ? (modelData.kind === "info" ? Theme.textRowH : Theme.rowH) : 0
                        visible: !modelData.visibleWhenKey || root.values[modelData.visibleWhenKey] === modelData.visibleWhenVal

                        EnumCombo {
                            width: parent.width
                            visible: modelData.kind === "combo"
                            label: modelData.label
                            options: modelData.options || []
                            optionsTr: modelData.optionsTr || []
                            value: root.values[modelData.key] !== undefined ? root.values[modelData.key] : ""
                            editable: modelData.editable !== undefined ? modelData.editable : false
                            onChanged: function (v) {
                                root.setField(modelData.key, v);
                            }
                        }

                        Toggle {
                            width: parent.width
                            visible: modelData.kind === "toggle"
                            label: modelData.label
                            hint: modelData.hint || ""
                            value: root.values[modelData.key] === "on"
                            // 双栏半宽列下缩小字体，避免窄列 label 挤换行；单栏满宽时 1.0 不变。
                            fontScale: parent && parent.width < Theme.twoColHalfWidth
                                       ? Theme.twoColFontScale : 1.0
                            onToggled: function (v) {
                                root.setField(modelData.key, v ? "on" : "off");
                            }
                        }

                        Row {
                            width: parent.width
                            spacing: Theme.gapM
                            visible: modelData.kind === "info"
                            Text {
                                width: parent.width * 0.42
                                text: modelData.label
                                color: Theme.fg
                                font.pixelSize: Theme.fontBody
                                elide: Text.ElideRight
                            }
                            Text {
                                text: modelData.value
                                color: Theme.muted
                                font.pixelSize: Theme.fontBody
                            }
                        }
                    }
                }

                Column {
                    id: fieldCol
                    width: parent.width
                    spacing: Theme.gapS

                    // 当前 fields 缓存：activeFields() 每次 find/filter 较重，binding 里复用一份。
                    // hasColumns 决定走双栏（Math / Other experimental，字段定义里标了 column 0/1）
                    // 还是单栏（General / Keyboard / Convert）。
                    property var currentFields: root.activeFields()
                    property bool hasColumns: {
                        var fs = currentFields;
                        for (var i = 0; i < fs.length; i++)
                            if (fs[i].column === 0 || fs[i].column === 1)
                                return true;
                        return false;
                    }

                    // 单栏：满宽 Repeater。
                    Repeater {
                        model: fieldCol.hasColumns ? [] : fieldCol.currentFields
                        delegate: fieldDelegate
                    }

                    // 双栏（Math / Other experimental）：字段定义是「整块 column 0 后整块
                    // column 1」（非交替），单 Repeater 平铺会上下错位，故按 column 拆左右两列并排。
                    // Toggle 内部 labelWidth/字体按 parent.width 缩放，半宽列下自动等比缩小。
                    Row {
                        width: fieldCol.width
                        spacing: Theme.twoColGap
                        visible: fieldCol.hasColumns
                        Column {
                            width: (fieldCol.width - Theme.twoColGap) / 2
                            spacing: Theme.gapS
                            Repeater {
                                model: fieldCol.hasColumns ? fieldCol.currentFields.filter(function (f) {
                                    return f.column === 0;
                                }) : []
                                delegate: fieldDelegate
                            }
                        }
                        Column {
                            width: (fieldCol.width - Theme.twoColGap) / 2
                            spacing: Theme.gapS
                            Repeater {
                                model: fieldCol.hasColumns ? fieldCol.currentFields.filter(function (f) {
                                    return f.column === 1;
                                }) : []
                                delegate: fieldDelegate
                            }
                        }
                    }
                }
            }
        }
    }

    // 返回当前应渲染的 fields 列表（由 activeTab 决定）：
    //   非 Convert tab -> activeTabObj.fields
    //   Convert tab -> activeSubTabObj.fields（子 tab 的 fields）
    function activeFields() {
        var tabObj = root.meta && root.meta.tabs ? root.meta.tabs.find(function (t) {
            return t.key === root.activeTab;
        }) : null;
        if (!tabObj)
            return [];
        if (root.activeTab === "convert" && tabObj.subTabs) {
            var subObj = tabObj.subTabs.find(function (s) {
                return s.key === root.activeSubTab;
            });
            return subObj ? (subObj.fields || []) : [];
        }
        return tabObj.fields || [];
    }
}
