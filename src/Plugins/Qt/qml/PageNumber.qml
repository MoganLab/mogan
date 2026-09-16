// PageNumber.qml — 「文档 → 页码」QML 对话框。
// DialogShell 外壳 + 纵向页面栏 + 操作面板 + DialogButtons。一次性提交：
// 打开时拉取当前规则与总页数（本地暂存），用户在弹窗内增删规则并实时预览，
// 点「应用」才一次性将规则写入文档 initial 并关窗；点「取消」丢弃。
//
// context property / bridge：
//   pnBridge     —— C++ bridge（PageNumberBridge），提供 meta() / submit(rules) / cancel() / startMove()
//   dialogButtons—— [应用, 取消]，已翻译
//   closeBridge / dpScale / isDark 共用属性

import QtQuick
import "atoms"

DialogShell {
    id: root
    implicitWidth: 920
    implicitHeight: 620
    implicitMargins: 20 * Theme.scaleFactor

    onActivate: () => root.submit()
    onCancel: () => root.cancel()

    property var meta: typeof pnBridge !== "undefined" ? pnBridge.meta() : ({
        total: 1,
        rules: [],
        labels: {}
    })
    property var labels: meta && meta.labels ? meta.labels : ({})
    property int totalPages: meta && meta.total ? Math.max(1, meta.total) : 1
    property var buttonLabels: typeof dialogButtons !== "undefined" ? dialogButtons : [
        labels.btnApply || qsTr("Apply"),
        labels.btnCancel || qsTr("Cancel")
    ]

    // rules 列表：[{ start: 1, end: 3, style: "roman" }, ...]
    property var rules: {
        var r = [];
        if (root.meta && root.meta.rules) {
            for (var i = 0; i < root.meta.rules.length; i++) {
                var item = root.meta.rules[i];
                r.push({
                    start: Number(item.start),
                    end: (item.end === "total" || Number(item.end) >= root.totalPages) ? "total" : Number(item.end),
                    style: item.style
                });
            }
        }
        return r;
    }

    property var pendingEnd: null
    property string curStyle: ""
    property string tipMessage: ""
    property bool dragging: false
    property int autoScrollDir: 0

    readonly property real rowH: 38 * Theme.scaleFactor
    readonly property var ruleColors: ["#d32f2f", "#7b61c9", "#f59e0b", "#3a7bd5", "#ea580c", "#0891b2"]

    // 样式选项定义
    readonly property var styleDefs: [
        { id: "blank",  sample: root.labels.styleBlankSample || qsTr("(hidden)"), name: root.labels.styleBlank || qsTr("Hide page numbers") },
        { id: "roman",  sample: "i, ii, iii", name: root.labels.styleRoman || "roman" },
        { id: "Roman",  sample: "I, II, III", name: root.labels.styleRomanUpper || "Roman" },
        { id: "hanzi",  sample: "一, 二, 三", name: root.labels.styleHanzi || qsTr("Chinese numerals") },
        { id: "arabic", sample: "1, 2, 3", name: root.labels.styleArabic || qsTr("Arabic numerals") }
    ]

    function styleName(id) {
        for (var i = 0; i < styleDefs.length; i++) {
            if (styleDefs[i].id === id) return styleDefs[i].name;
        }
        return id;
    }

    function minStart() {
        if (rules.length === 0) return 1;
        var last = rules[rules.length - 1];
        if (last.end === "total" || Number(last.end) >= totalPages) return totalPages + 1;
        return Number(last.end) + 1;
    }

    property int pendingStart: minStart()

    onRulesChanged: {
        pendingStart = minStart();
        pendingEnd = null;
        tipMessage = "";
    }

    onPendingEndChanged: {
        if (!root.dragging && root.pendingEnd !== null) {
            var targetY = (Math.min(root.totalPages, root.pendingEnd) - 1) * root.rowH;
            if (targetY < stripFlickable.contentY || targetY > stripFlickable.contentY + stripFlickable.height - root.rowH) {
                stripFlickable.contentY = Math.max(0, Math.min(stripFlickable.contentHeight - stripFlickable.height, targetY - stripFlickable.height / 2));
            }
        }
    }

    onPendingStartChanged: {
        if (curStyle === "arabic" && tipMessage !== "") {
            tipMessage = (labels.arabicTip || qsTr("Defaulted from page %1 to the end (last page + 1000) so that page number 1 starts from page %1.")).arg(root.pendingStart);
        }
    }

    function done() {
        return minStart() > totalPages;
    }

    function ruleOfPage(p) {
        for (var i = 0; i < rules.length; i++) {
            var s = Number(rules[i].start);
            var e = (rules[i].end === "total" || Number(rules[i].end) >= totalPages) ? totalPages : Number(rules[i].end);
            if (p >= s && p <= e) return i;
        }
        return -1;
    }

    function toRoman(n) {
        var T = [[1000, "M"], [900, "CM"], [500, "D"], [400, "CD"], [100, "C"],
                 [90, "XC"], [50, "L"], [40, "XL"], [10, "X"], [9, "IX"],
                 [5, "V"], [4, "IV"], [1, "I"]];
        var s = "";
        for (var i = 0; i < T.length; i++) {
            var v = T[i][0];
            var r = T[i][1];
            while (n >= v) { s += r; n -= v; }
        }
        return s;
    }

    function toHanzi(n) {
        var H = ["零", "一", "二", "三", "四", "五", "六", "七", "八", "九"];
        if (n <= 0) return "";
        if (n < 10) return H[n];
        if (n === 10) return "十";
        if (n < 20) return "十" + H[n % 10];
        if (n < 100) {
            var tens = Math.floor(n / 10);
            var rem = n % 10;
            return H[tens] + "十" + (rem > 0 ? H[rem] : "");
        }
        return String(n);
    }

    function formatNr(n, style) {
        if (n < 1) return "";
        if (style === "arabic") return String(n);
        if (style === "roman") return toRoman(n).toLowerCase();
        if (style === "Roman") return toRoman(n);
        if (style === "hanzi") return toHanzi(n);
        return "";
    }

    function pageDisplay(p) {
        for (var i = 0; i < rules.length; i++) {
            var s = Number(rules[i].start);
            var e = (rules[i].end === "total" || Number(rules[i].end) >= totalPages) ? totalPages : Number(rules[i].end);
            if (p >= s && p <= e) {
                if (rules[i].style === "blank") return "";
                return formatNr(p - s + 1, rules[i].style);
            }
        }
        return String(p);
    }

    function endText(end) {
        if (end === "total" || Number(end) >= totalPages) {
            return totalPages + "（" + (labels.toEnd || qsTr("to end of document")) + "）";
        }
        return String(end);
    }

    function addRule() {
        if (!pendingEnd || !curStyle || done()) return;
        var cur = [];
        for (var i = 0; i < rules.length; i++) cur.push(rules[i]);
        cur.push({
            start: pendingStart,
            end: (pendingEnd >= totalPages ? "total" : pendingEnd),
            style: curStyle
        });
        rules = cur;
        pendingStart = minStart();
        pendingEnd = null;
        tipMessage = "";
    }

    function deleteLastRule() {
        if (rules.length === 0) return;
        var cur = [];
        for (var i = 0; i < rules.length - 1; i++) cur.push(rules[i]);
        rules = cur;
        pendingStart = minStart();
        pendingEnd = null;
        tipMessage = "";
    }

    function submit() {
        var out = [];
        for (var i = 0; i < rules.length; i++) {
            out.push({
                start: String(rules[i].start),
                end: String(rules[i].end),
                style: String(rules[i].style)
            });
        }
        if (typeof pnBridge !== "undefined") {
            pnBridge.submit(out);
        }
    }

    function cancel() {
        if (typeof pnBridge !== "undefined") {
            pnBridge.cancel();
        } else if (typeof closeBridge !== "undefined") {
            closeBridge.cancel();
        }
    }

    Timer {
        id: autoScrollTimer
        interval: 30
        repeat: true
        running: false
        onTriggered: {
            if (stripFlickable.contentHeight > stripFlickable.height) {
                var nextY = stripFlickable.contentY + root.autoScrollDir * (15 * Theme.scaleFactor);
                stripFlickable.contentY = Math.max(0, Math.min(stripFlickable.contentHeight - stripFlickable.height, nextY));
            }
        }
    }

    content: Column {
        anchors.fill: parent
        spacing: 12 * Theme.scaleFactor

        // Header (拖动标题区)
        Item {
            width: parent.width
            height: 36 * Theme.scaleFactor

            MouseArea {
                anchors.fill: parent
                onPressed: if (typeof pnBridge !== "undefined") pnBridge.startMove()
            }

            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12 * Theme.scaleFactor

                Text {
                    text: root.labels.title || qsTr("Page number settings")
                    font.pixelSize: 17 * Theme.scaleFactor
                    font.bold: true
                    color: Theme.fg
                }

                Text {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 2 * Theme.scaleFactor
                    text: root.labels.hint || qsTr("All pages are arranged vertically and scrollable; drag handle to select range, auto-scrolls near edge")
                    font.pixelSize: 12 * Theme.scaleFactor
                    color: Theme.muted
                }
            }
        }

        // 分割线
        Rectangle {
            width: parent.width
            height: Theme.borderW
            color: Theme.borderClr
        }

        // 主体区域：左侧页面条 + 右侧操作面板
        Row {
            width: parent.width
            height: parent.height - 36 * Theme.scaleFactor - 12 * Theme.scaleFactor - Theme.borderW - 48 * Theme.scaleFactor
            spacing: 0

            // 左侧：页面纵向条
            Rectangle {
                id: stripContainer
                width: 220 * Theme.scaleFactor
                height: parent.height
                color: Theme.bg
                clip: true

                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: Theme.borderW
                    color: Theme.borderClr
                }

                Flickable {
                    id: stripFlickable
                    anchors.fill: parent
                    anchors.rightMargin: 1 * Theme.scaleFactor
                    contentWidth: width
                    contentHeight: root.totalPages * root.rowH
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    Item {
                        id: stripContent
                        width: parent.width
                        height: root.totalPages * root.rowH

                        // 页面列表
                        Repeater {
                            model: root.totalPages
                            delegate: Rectangle {
                                id: rowItem
                                readonly property int pageNum: index + 1
                                readonly property int rIdx: root.ruleOfPage(pageNum)
                                readonly property bool isPending: !root.done() && root.pendingEnd !== null &&
                                                                  pageNum >= root.pendingStart && pageNum <= root.pendingEnd
                                readonly property string disp: root.pageDisplay(pageNum)

                                x: 0
                                y: index * root.rowH
                                width: stripFlickable.width
                                height: root.rowH
                                color: isPending ? (Theme.dark ? "#1f4a48" : "#dff3f1") : (maRow.containsMouse ? Theme.fieldBgHover : "transparent")

                                // 左侧色带（已添加规则的标识）
                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    width: 4 * Theme.scaleFactor
                                    color: rowItem.rIdx >= 0 ? root.ruleColors[rowItem.rIdx % root.ruleColors.length] :
                                           (rowItem.isPending ? (Theme.dark ? "#2791ad" : "#215a6a") : "transparent")
                                }

                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.leftMargin: 12 * Theme.scaleFactor
                                    spacing: 8 * Theme.scaleFactor

                                    // 页面缩略图占位
                                    Rectangle {
                                        width: 24 * Theme.scaleFactor
                                        height: 24 * Theme.scaleFactor
                                        radius: 3 * Theme.scaleFactor
                                        color: Theme.dark ? "#3a3a3a" : "#ffffff"
                                        border.width: Theme.borderW
                                        border.color: Theme.borderClr

                                        Rectangle {
                                            anchors.bottom: parent.bottom
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            height: 7 * Theme.scaleFactor
                                            radius: 2 * Theme.scaleFactor
                                            color: Theme.dark ? "#4a4a4a" : "#f0f2f5"
                                        }
                                    }

                                    // 物理页号
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: (root.labels.physPage || qsTr("Page %1")).arg(rowItem.pageNum)
                                        font.pixelSize: 12 * Theme.scaleFactor
                                        color: Theme.muted
                                    }

                                    // 显示页码
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: rowItem.disp === "" ? "–" : rowItem.disp
                                        font.pixelSize: 14 * Theme.scaleFactor
                                        font.bold: rowItem.disp !== ""
                                        color: rowItem.disp === "" ? Theme.muted : Theme.fg
                                    }
                                }

                                MouseArea {
                                    id: maRow
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    preventStealing: true
                                    cursorShape: (!root.done() && rowItem.pageNum >= root.pendingStart) ? Qt.PointingHandCursor : Qt.ArrowCursor
                                    onPressed: {
                                        if (!root.done() && rowItem.pageNum >= root.pendingStart) {
                                            root.pendingEnd = rowItem.pageNum;
                                            root.dragging = true;
                                        }
                                    }
                                    onReleased: {
                                        root.dragging = false;
                                        autoScrollTimer.stop();
                                    }
                                    onPositionChanged: function (mouse) {
                                        if (!pressed) return;
                                        var pInContent = mapToItem(stripContent, mouse.x, mouse.y);
                                        var p = Math.floor(pInContent.y / root.rowH) + 1;
                                        if (p < root.pendingStart) p = root.pendingStart;
                                        if (p > root.totalPages) p = root.totalPages;
                                        root.pendingEnd = p;
                                    }
                                }
                            }
                        }

                        // 拖动手柄
                        Rectangle {
                            id: dragHandle
                            visible: !root.done()
                            z: 20
                            readonly property int targetPage: root.pendingEnd !== null ? Math.min(root.totalPages, root.pendingEnd) : (root.pendingStart - 1)
                            x: 8 * Theme.scaleFactor
                            y: Math.max(0, Math.min(root.totalPages * root.rowH - height,
                                                    (targetPage >= root.pendingStart ? targetPage * root.rowH : (root.pendingStart - 1) * root.rowH + root.rowH) - height / 2))
                            width: stripFlickable.width - 16 * Theme.scaleFactor
                            height: 14 * Theme.scaleFactor
                            radius: height / 2
                            color: Theme.dark ? "#2791ad" : "#215a6a"

                            Text {
                                anchors.centerIn: parent
                                text: "⋮⋮"
                                color: "#ffffff"
                                font.pixelSize: 10 * Theme.scaleFactor
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.SizeVerCursor
                                hoverEnabled: true
                                preventStealing: true
                                onPressed: root.dragging = true
                                onReleased: {
                                    root.dragging = false;
                                    autoScrollTimer.stop();
                                }
                                onPositionChanged: function (mouse) {
                                    if (!pressed) return;
                                    var pInContent = mapToItem(stripContent, mouse.x, mouse.y);
                                    var p = Math.floor(pInContent.y / root.rowH) + 1;
                                    if (p < root.pendingStart) p = root.pendingStart;
                                    if (p > root.totalPages) p = root.totalPages;
                                    root.pendingEnd = p;

                                    // 边缘自动滚动
                                    var viewY = pInContent.y - stripFlickable.contentY;
                                    if (viewY > stripFlickable.height - 40 * Theme.scaleFactor) {
                                        root.autoScrollDir = 1;
                                        autoScrollTimer.start();
                                    } else if (viewY < 40 * Theme.scaleFactor) {
                                        root.autoScrollDir = -1;
                                        autoScrollTimer.start();
                                    } else {
                                        autoScrollTimer.stop();
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 右侧：操作面板
            Flickable {
                width: parent.width - stripContainer.width
                height: parent.height
                contentWidth: width
                contentHeight: rightCol.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                Column {
                    id: rightCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 24 * Theme.scaleFactor
                    anchors.rightMargin: 24 * Theme.scaleFactor
                    anchors.top: parent.top
                    anchors.topMargin: 4 * Theme.scaleFactor
                    spacing: 16 * Theme.scaleFactor

                    // 1. 新规则的范围
                    Column {
                        width: parent.width
                        spacing: 10 * Theme.scaleFactor

                        Row {
                            spacing: 8 * Theme.scaleFactor
                            Text {
                                text: root.labels.secRange || qsTr("① Range for new rule")
                                font.pixelSize: 13 * Theme.scaleFactor
                                font.bold: true
                                color: Theme.fg
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.labels.subRange || qsTr("Directly enter page range, click [to end of document], or drag handle")
                                font.pixelSize: 12 * Theme.scaleFactor
                                color: Theme.muted
                            }
                        }

                        // 如果全部页面已被覆盖
                        Text {
                            visible: root.done()
                            text: root.labels.rangeDone || qsTr("All pages covered by rules")
                            font.pixelSize: 13 * Theme.scaleFactor
                            color: Theme.muted
                        }

                        // 实时范围清晰显示
                        Text {
                            visible: !root.done()
                            text: {
                                var ps = root.pendingStart;
                                if (root.pendingEnd !== null) {
                                    var pe = root.pendingEnd;
                                    var t = root.endText(pe);
                                    var cnt = Math.min(root.totalPages, pe) - ps + 1;
                                    return (root.labels.rangeUpto || qsTr("Page %1 ~ Page %2 (%3 pages)")).arg(ps).arg(t).arg(cnt);
                                }
                                return (root.labels.rangePrompt || qsTr("Starting from page %1, drag handle to select end page")).arg(ps);
                            }
                            font.pixelSize: 13 * Theme.scaleFactor
                            font.bold: root.pendingEnd !== null
                            color: root.pendingEnd !== null ? (Theme.dark ? "#bfeeeb" : "#215a6a") : Theme.fg
                        }

                        // 范围选择交互行
                        Row {
                            visible: !root.done()
                            spacing: 6 * Theme.scaleFactor

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.labels.fromPage || qsTr("From page")
                                font.pixelSize: 13 * Theme.scaleFactor
                                color: Theme.fg
                            }

                            // 起始页输入框
                            Rectangle {
                                width: 52 * Theme.scaleFactor
                                height: 32 * Theme.scaleFactor
                                radius: 6 * Theme.scaleFactor
                                color: Theme.fieldBg
                                border.width: Theme.borderW
                                border.color: startInput.activeFocus ? (Theme.dark ? "#2791ad" : "#215a6a") : Theme.borderClr

                                TextInput {
                                    id: startInput
                                    anchors.fill: parent
                                    anchors.margins: 4 * Theme.scaleFactor
                                    horizontalAlignment: TextInput.AlignHCenter
                                    verticalAlignment: TextInput.AlignVCenter
                                    font.pixelSize: 13 * Theme.scaleFactor
                                    color: Theme.fg
                                    selectByMouse: true
                                    text: String(root.pendingStart)
                                    validator: IntValidator {
                                        bottom: root.minStart()
                                        top: root.totalPages
                                    }
                                    onTextEdited: {
                                        var v = parseInt(text);
                                        if (!isNaN(v) && v >= root.minStart() && v <= root.totalPages) {
                                            root.pendingStart = v;
                                            if (root.pendingEnd !== null && root.pendingEnd < v) {
                                                root.pendingEnd = v;
                                            }
                                        }
                                    }
                                    onActiveFocusChanged: {
                                        if (!activeFocus) text = String(root.pendingStart);
                                    }
                                }
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: (root.labels.pageUnit || qsTr("page")) + "  " + (root.labels.toPage || qsTr("to page"))
                                font.pixelSize: 13 * Theme.scaleFactor
                                color: Theme.fg
                            }

                            // 结束页输入框 + Steppers
                            Row {
                                spacing: 3 * Theme.scaleFactor

                                Rectangle {
                                    width: 58 * Theme.scaleFactor
                                    height: 32 * Theme.scaleFactor
                                    radius: 6 * Theme.scaleFactor
                                    color: Theme.fieldBg
                                    border.width: Theme.borderW
                                    border.color: endInput.activeFocus ? (Theme.dark ? "#2791ad" : "#215a6a") : Theme.borderClr

                                    TextInput {
                                        id: endInput
                                        anchors.fill: parent
                                        anchors.margins: 4 * Theme.scaleFactor
                                        horizontalAlignment: TextInput.AlignHCenter
                                        verticalAlignment: TextInput.AlignVCenter
                                        font.pixelSize: 13 * Theme.scaleFactor
                                        color: Theme.fg
                                        selectByMouse: true
                                        text: root.pendingEnd !== null ? (root.pendingEnd > root.totalPages ? String(root.totalPages) : String(root.pendingEnd)) : ""
                                        validator: IntValidator {
                                            bottom: root.pendingStart
                                            top: root.totalPages + 1000
                                        }
                                        onTextEdited: {
                                            var v = parseInt(text);
                                            if (!isNaN(v) && v >= root.pendingStart && v <= root.totalPages + 1000) {
                                                root.pendingEnd = v;
                                            }
                                        }
                                        onActiveFocusChanged: {
                                            if (!activeFocus) {
                                                text = root.pendingEnd !== null ? (root.pendingEnd > root.totalPages ? String(root.totalPages) : String(root.pendingEnd)) : "";
                                            }
                                        }
                                    }
                                }

                                // 减页按钮 [−]
                                Rectangle {
                                    width: 26 * Theme.scaleFactor
                                    height: 32 * Theme.scaleFactor
                                    radius: 6 * Theme.scaleFactor
                                    color: btnDecMa.containsMouse ? Theme.fieldBgHover : Theme.fieldBg
                                    border.width: Theme.borderW
                                    border.color: Theme.borderClr

                                    Text {
                                        anchors.centerIn: parent
                                        text: "−"
                                        font.pixelSize: 14 * Theme.scaleFactor
                                        color: Theme.fg
                                    }

                                    MouseArea {
                                        id: btnDecMa
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            var cur = root.pendingEnd !== null ? root.pendingEnd : root.pendingStart;
                                            if (cur > root.pendingStart) {
                                                root.pendingEnd = cur - 1;
                                            }
                                        }
                                    }
                                }

                                // 加页按钮 [+]
                                Rectangle {
                                    width: 26 * Theme.scaleFactor
                                    height: 32 * Theme.scaleFactor
                                    radius: 6 * Theme.scaleFactor
                                    color: btnIncMa.containsMouse ? Theme.fieldBgHover : Theme.fieldBg
                                    border.width: Theme.borderW
                                    border.color: Theme.borderClr

                                    Text {
                                        anchors.centerIn: parent
                                        text: "+"
                                        font.pixelSize: 14 * Theme.scaleFactor
                                        color: Theme.fg
                                    }

                                    MouseArea {
                                        id: btnIncMa
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            var cur = root.pendingEnd !== null ? root.pendingEnd : (root.pendingStart - 1);
                                            if (cur < root.totalPages) {
                                                root.pendingEnd = cur + 1;
                                            }
                                        }
                                    }
                                }
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.labels.pageUnit || qsTr("page")
                                font.pixelSize: 13 * Theme.scaleFactor
                                color: Theme.fg
                            }

                            // [至文末] 快捷按钮
                            Rectangle {
                                height: 32 * Theme.scaleFactor
                                width: toEndText.implicitWidth + 16 * Theme.scaleFactor
                                radius: 6 * Theme.scaleFactor
                                color: toEndMa.containsMouse ? Theme.fieldBgHover : Theme.fieldBg
                                border.width: Theme.borderW
                                border.color: (root.pendingEnd !== null && root.pendingEnd >= root.totalPages) ? (Theme.dark ? "#2791ad" : "#215a6a") : Theme.borderClr

                                Text {
                                    id: toEndText
                                    anchors.centerIn: parent
                                    text: root.labels.toEnd || qsTr("to end of document")
                                    font.pixelSize: 12 * Theme.scaleFactor
                                    color: (root.pendingEnd !== null && root.pendingEnd >= root.totalPages) ? (Theme.dark ? "#2791ad" : "#215a6a") : Theme.fg
                                    font.bold: root.pendingEnd !== null && root.pendingEnd >= root.totalPages
                                }

                                MouseArea {
                                    id: toEndMa
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.pendingEnd = root.totalPages + 1000;
                                    }
                                }
                            }

                            // （共 N 页）
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: root.pendingEnd !== null
                                text: {
                                    if (root.pendingEnd === null) return "";
                                    var count = Math.min(root.totalPages, root.pendingEnd) - root.pendingStart + 1;
                                    return "（共 " + count + " 页）";
                                }
                                font.pixelSize: 12 * Theme.scaleFactor
                                color: Theme.muted
                            }
                        }
                    }

                    // 2. 选择页码样式
                    Column {
                        width: parent.width
                        spacing: 8 * Theme.scaleFactor

                        Text {
                            text: root.labels.secStyle || qsTr("② Choose page number style")
                            font.pixelSize: 13 * Theme.scaleFactor
                            font.bold: true
                            color: Theme.fg
                        }

                        Flow {
                            width: parent.width
                            spacing: 8 * Theme.scaleFactor

                            Repeater {
                                model: root.styleDefs
                                delegate: Rectangle {
                                    id: chip
                                    readonly property bool isSelected: root.curStyle === modelData.id
                                    readonly property bool isDisabled: root.done()

                                    width: Math.max(92 * Theme.scaleFactor, chipCol.implicitWidth + 24 * Theme.scaleFactor)
                                    height: chipCol.implicitHeight + 14 * Theme.scaleFactor
                                    radius: 8 * Theme.scaleFactor
                                    opacity: isDisabled ? 0.4 : 1.0
                                    color: isSelected ? (Theme.dark ? "#1f4a48" : "#dff3f1") :
                                           (chipMa.containsMouse && !isDisabled ? Theme.fieldBgHover : Theme.fieldBg)
                                    border.width: isSelected ? 1.5 * Theme.scaleFactor : Theme.borderW
                                    border.color: isSelected ? (Theme.dark ? "#2791ad" : "#215a6a") :
                                                  (chipMa.containsMouse && !isDisabled ? (Theme.dark ? "#666" : "#aaa") : Theme.borderClr)

                                    Column {
                                        id: chipCol
                                        anchors.centerIn: parent
                                        spacing: 2 * Theme.scaleFactor

                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: modelData.sample
                                            font.pixelSize: 13 * Theme.scaleFactor
                                            font.bold: chip.isSelected
                                            color: Theme.fg
                                        }
                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: modelData.name
                                            font.pixelSize: 11 * Theme.scaleFactor
                                            color: Theme.muted
                                        }
                                    }

                                    MouseArea {
                                        id: chipMa
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: chip.isDisabled ? Qt.ArrowCursor : Qt.PointingHandCursor
                                        onClicked: {
                                            if (!chip.isDisabled) {
                                                root.curStyle = modelData.id;
                                                if (modelData.id === "arabic") {
                                                    root.pendingEnd = root.totalPages + 1000;
                                                    root.tipMessage = (root.labels.arabicTip || qsTr("Defaulted from page %1 to the end (last page + 1000) so that page number 1 starts from page %1.")).arg(root.pendingStart);
                                                } else {
                                                    root.pendingEnd = root.pendingStart;
                                                    if (modelData.id === "roman" || modelData.id === "Roman") {
                                                        root.tipMessage = root.labels.romanTip || qsTr("Roman numeral page numbering (i, ii... or I, II...) is generally used for the table of contents and preface of books.");
                                                    } else {
                                                        root.tipMessage = "";
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 温馨提示条
                    Rectangle {
                        visible: root.tipMessage !== "" && !root.done()
                        width: parent.width
                        height: tipRow.implicitHeight + 16 * Theme.scaleFactor
                        radius: 8 * Theme.scaleFactor
                        color: Theme.dark ? "#263529" : "#eef8f2"
                        border.width: Theme.borderW
                        border.color: Theme.dark ? "#2f6a67" : "#a3d9c9"

                        Row {
                            id: tipRow
                            anchors.fill: parent
                            anchors.margins: 8 * Theme.scaleFactor
                            spacing: 8 * Theme.scaleFactor

                            Text {
                                text: "💡"
                                font.pixelSize: 13 * Theme.scaleFactor
                            }

                            Text {
                                width: parent.width - 28 * Theme.scaleFactor
                                text: root.tipMessage
                                font.pixelSize: 12 * Theme.scaleFactor
                                color: Theme.dark ? "#bfeeeb" : "#194f53"
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    // 添加规则按钮
                    Rectangle {
                        readonly property bool canAdd: !root.done() && root.pendingEnd !== null && root.curStyle !== ""
                        width: addBtnText.implicitWidth + 28 * Theme.scaleFactor
                        height: 32 * Theme.scaleFactor
                        radius: 7 * Theme.scaleFactor
                        opacity: canAdd ? 1.0 : 0.45
                        color: (addBtnMa.containsMouse && canAdd) ? (Theme.dark ? "#2f9ebc" : "#1d4f5d") : (Theme.dark ? "#2791ad" : "#215a6a")

                        Text {
                            id: addBtnText
                            anchors.centerIn: parent
                            text: root.labels.btnAddRule || qsTr("+ Add rule")
                            font.pixelSize: 13 * Theme.scaleFactor
                            font.bold: true
                            color: "#ffffff"
                        }

                        MouseArea {
                            id: addBtnMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: parent.canAdd ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: {
                                if (parent.canAdd) root.addRule();
                            }
                        }
                    }

                    // 3. 已添加的规则
                    Column {
                        width: parent.width
                        spacing: 8 * Theme.scaleFactor

                        Row {
                            spacing: 8 * Theme.scaleFactor
                            Text {
                                text: root.labels.secRules || qsTr("Added rules")
                                font.pixelSize: 13 * Theme.scaleFactor
                                font.bold: true
                                color: Theme.fg
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.labels.subRules || qsTr("Rules connect sequentially; only the last rule can be deleted")
                                font.pixelSize: 12 * Theme.scaleFactor
                                color: Theme.muted
                            }
                        }

                        // 规则列表
                        Column {
                            width: parent.width
                            spacing: 6 * Theme.scaleFactor

                            Text {
                                visible: root.rules.length === 0
                                text: (root.labels.emptyRules || qsTr("No rules yet — all pages numbered continuously")).arg(root.totalPages)
                                font.pixelSize: 12 * Theme.scaleFactor
                                color: Theme.muted
                            }

                            Repeater {
                                model: root.rules
                                delegate: Rectangle {
                                    readonly property bool isLast: index === root.rules.length - 1
                                    width: parent.width
                                    height: 36 * Theme.scaleFactor
                                    radius: 7 * Theme.scaleFactor
                                    color: Theme.dark ? "#333333" : "#fafbfc"
                                    border.width: Theme.borderW
                                    border.color: Theme.borderClr

                                    // 左侧彩色标识条
                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        width: 4 * Theme.scaleFactor
                                        radius: 2 * Theme.scaleFactor
                                        color: root.ruleColors[index % root.ruleColors.length]
                                    }

                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: 12 * Theme.scaleFactor
                                        anchors.right: delBtn.left
                                        anchors.rightMargin: 8 * Theme.scaleFactor
                                        spacing: 6 * Theme.scaleFactor

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: {
                                                var s = modelData.start;
                                                var e = root.endText(modelData.end);
                                                var name = root.styleName(modelData.style);
                                                var base = (root.labels.physPage || qsTr("Page %1")).arg(s) + " ~ " + (root.labels.physPage || qsTr("Page %1")).arg(e) + " · " + name;
                                                if (modelData.style !== "blank") {
                                                    base += "（" + (root.labels.fromSample || qsTr("from %1")).arg(root.formatNr(1, modelData.style)) + "）";
                                                }
                                                return base;
                                            }
                                            font.pixelSize: 13 * Theme.scaleFactor
                                            color: Theme.fg
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // 删除按钮（仅最后一条可点）
                                    Rectangle {
                                        id: delBtn
                                        anchors.right: parent.right
                                        anchors.rightMargin: 8 * Theme.scaleFactor
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 24 * Theme.scaleFactor
                                        height: 24 * Theme.scaleFactor
                                        radius: 4 * Theme.scaleFactor
                                        color: (delMa.containsMouse && isLast) ? (Theme.dark ? "#4a2020" : "#fdecea") : "transparent"

                                        Text {
                                            anchors.centerIn: parent
                                            text: "✕"
                                            font.pixelSize: 13 * Theme.scaleFactor
                                            color: isLast ? ((delMa.containsMouse) ? "#c0392b" : Theme.muted) : (Theme.dark ? "#555" : "#ccc")
                                        }

                                        MouseArea {
                                            id: delMa
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: isLast ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            onClicked: {
                                                if (isLast) root.deleteLastRule();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 底部按钮栏
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 4 * Theme.scaleFactor
            spacing: Theme.gapM

            DialogButtons {
                buttonLabels: root.buttonLabels
                onClicked: function (idx) {
                    if (idx === 0) root.submit();
                    else root.cancel();
                }
            }
        }
    }
}
