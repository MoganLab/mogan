// OutlineSidebar.qml — 现代化 STEM 左侧目录树 QML 侧边栏
// 支持 PDF 书签与 TMU/STEM 文档章节大纲
// 全面适配浅色与深色模式，优雅层级缩进、右侧对齐页码、三点更多菜单（展开/折叠）、搜索实时过滤

import QtQuick
import "atoms"

Item {
    id: root

    // 数据源与交互桥接对象
    readonly property var bridge: (typeof outlineBridge !== "undefined" && outlineBridge) ? outlineBridge : null
    readonly property var rawModel: bridge ? bridge.outlineModel : []
    readonly property string activeId: (bridge && bridge.currentId) ? bridge.currentId : ""

    // 内部展开状态映射表：node.id -> bool
    property var expandedMap: ({})
    property string searchQuery: ""
    property bool menuOpen: false

    // 扁平化渲染列表
    property var displayItems: []

    function isExpanded(id) {
        return expandedMap[id] !== undefined ? expandedMap[id] : true;
    }

    function updateDisplayItems() {
        let q = searchQuery.trim().toLowerCase();
        let result = [];

        // 单遍自底向上收集：返回子树是否有匹配项，避免按节点重复扫描子树
        function processNodes(nodes, level, out) {
            let anyMatch = false;
            let count = nodes ? nodes.length : 0;
            for (let i = 0; i < count; ++i) {
                let node = nodes[i];
                let hasKids = node.children && node.children.length > 0;
                let selfMatch = !q || (node.title && node.title.toLowerCase().indexOf(q) !== -1);
                let expanded = isExpanded(node.id);

                // 搜索时必须下钻整棵子树找匹配；非搜索时仅展开态需要子项
                let childItems = [];
                let childMatch = false;
                if (hasKids && (q || expanded)) {
                    childMatch = processNodes(node.children, level + 1, childItems);
                }

                if (!q || selfMatch || childMatch) {
                    let expandedInView = q ? (childMatch || expanded) : expanded;
                    out.push({
                        id: node.id,
                        title: node.title || "",
                        target: node.target || "",
                        page: node.page || "",
                        level: level,
                        isLast: (i === count - 1),
                        hasChildren: hasKids,
                        expanded: expandedInView
                    });

                    // 仅当节点展开时，才把子节点放进渲染列表
                    if (hasKids && expandedInView) {
                        for (let j = 0; j < childItems.length; ++j) {
                            out.push(childItems[j]);
                        }
                    }
                    anyMatch = true;
                }
            }
            return anyMatch;
        }

        processNodes(rawModel, 0, result);
        displayItems = result;
    }

    onRawModelChanged: {
        updateDisplayItems();
    }

    onSearchQueryChanged: {
        updateDisplayItems();
    }

    // 单项折叠/展开（基于严格唯一的 node.id）
    function toggleExpand(nodeId) {
        let newMap = Object.assign({}, expandedMap);
        newMap[nodeId] = !isExpanded(nodeId);
        expandedMap = newMap;
        updateDisplayItems();
    }

    // 全部折叠 / 全部展开
    function setAllExpanded(expanded) {
        let newMap = {};
        function walk(nodes) {
            if (!nodes) return;
            for (let i = 0; i < nodes.length; ++i) {
                newMap[nodes[i].id] = expanded;
                if (nodes[i].children) walk(nodes[i].children);
            }
        }
        walk(rawModel);
        expandedMap = newMap;
        menuOpen = false;
        updateDisplayItems();
    }

    // 主题配色
    readonly property color sidebarBg: Theme.dark ? "#18181b" : "#ffffff"
    readonly property color borderRightClr: Theme.dark ? "#27272a" : "#e5e7eb"
    readonly property color headerTitleClr: Theme.dark ? "#f4f4f5" : "#1f2328"
    readonly property color itemHoverBg: Theme.dark ? "#27272a" : "#f0f2f5"
    readonly property color itemSelectBg: "#0284c7"
    readonly property color itemSelectFg: "#ffffff"
    readonly property color searchBg: Theme.dark ? "#27272a" : "#ffffff"
    readonly property color searchBorder: Theme.dark ? "#3f3f46" : "#d0d7de"
    readonly property color branchLineClr: Theme.dark ? "#3f3f46" : "#e5e7eb"
    readonly property color chevronClr: Theme.dark ? "#a1a1aa" : "#656d76"
    readonly property color pageNumClr: Theme.dark ? "#a1a1aa" : "#656d76"
    readonly property color menuBg: Theme.dark ? "#27272a" : "#ffffff"
    readonly property color menuBorder: Theme.dark ? "#3f3f46" : "#d0d7de"

    Rectangle {
        id: bgRect
        anchors.fill: parent
        color: sidebarBg

        // 右边框分割线
        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 1 * Theme.scaleFactor
            color: borderRightClr
        }

        // 全局空白处点击收起弹出菜单
        MouseArea {
            id: outsideMouse
            anchors.fill: parent
            visible: menuOpen
            z: 80
            onClicked: menuOpen = false
        }

        // 1. 顶部标题栏（Contents / 目录）
        Item {
            id: headerItem
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 38 * Theme.scaleFactor

            Text {
                id: titleText
                text: qsTr("Contents")
                anchors.centerIn: parent
                font.pixelSize: 14 * Theme.scaleFactor
                font.weight: Font.DemiBold
                color: headerTitleClr
            }

            // 右上角关闭按钮
            Rectangle {
                id: closeBtn
                width: 22 * Theme.scaleFactor
                height: 22 * Theme.scaleFactor
                radius: 4 * Theme.scaleFactor
                anchors.right: parent.right
                anchors.rightMargin: 12 * Theme.scaleFactor
                anchors.verticalCenter: parent.verticalCenter
                color: closeMouse.containsMouse ? (Theme.dark ? "#27272a" : "#eaecef") : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "×"
                    font.pixelSize: 16 * Theme.scaleFactor
                    color: Theme.dark ? "#a1a1aa" : "#57606a"
                }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (bridge) bridge.closeOutline();
                    }
                }
            }
        }

        // 2. 搜索栏 + 右侧「···」更多菜单按钮
        // 左右边距与 PDF 工具栏缩放框（100%）的左侧留白保持一致，不贴边
        Item {
            id: searchRow
            anchors.top: headerItem.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 8 * Theme.scaleFactor
            anchors.rightMargin: 8 * Theme.scaleFactor
            height: 32 * Theme.scaleFactor

            // 三点「···」菜单按钮（固定在最右侧，居中三颗圆点）
            Rectangle {
                id: moreBtn
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 28 * Theme.scaleFactor
                height: 28 * Theme.scaleFactor
                radius: 4 * Theme.scaleFactor
                color: moreMouse.containsMouse || menuOpen ? (Theme.dark ? "#3f3f46" : "#f0f2f5") : "transparent"
                border.width: 1 * Theme.scaleFactor
                border.color: menuOpen ? (Theme.dark ? "#38bdf8" : "#0284c7") : searchBorder

                Row {
                    anchors.centerIn: parent
                    spacing: 2.5 * Theme.scaleFactor
                    Repeater {
                        model: 3
                        Rectangle {
                            width: 3 * Theme.scaleFactor
                            height: 3 * Theme.scaleFactor
                            radius: width / 2
                            color: Theme.dark ? "#d4d4d8" : "#57606a"
                        }
                    }
                }

                MouseArea {
                    id: moreMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        menuOpen = !menuOpen;
                    }
                }
            }

            // 搜索输入框（自动填充到 moreBtn 左侧）
            Rectangle {
                id: searchBox
                anchors.left: parent.left
                anchors.right: moreBtn.left
                anchors.rightMargin: 6 * Theme.scaleFactor
                anchors.verticalCenter: parent.verticalCenter
                height: 28 * Theme.scaleFactor
                radius: 4 * Theme.scaleFactor
                color: searchBg
                border.width: 1 * Theme.scaleFactor
                border.color: searchInput.activeFocus ? (Theme.dark ? "#38bdf8" : "#0284c7") : searchBorder

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 8 * Theme.scaleFactor
                    anchors.rightMargin: 6 * Theme.scaleFactor
                    spacing: 6 * Theme.scaleFactor

                    TextInput {
                        id: searchInput
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 24 * Theme.scaleFactor
                        font.pixelSize: 12 * Theme.scaleFactor
                        color: Theme.fg
                        clip: true
                        onTextChanged: {
                            searchDebounce.restart();
                        }

                        // 搜索防抖：连续输入时只在停顿后重建渲染列表
                        Timer {
                            id: searchDebounce
                            interval: 200
                            onTriggered: root.searchQuery = searchInput.text
                        }

                        Text {
                            text: qsTr("Search...")
                            anchors.fill: parent
                            font.pixelSize: 12 * Theme.scaleFactor
                            color: Theme.muted
                            visible: !searchInput.text && !searchInput.activeFocus
                        }
                    }

                    // 清空按钮
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "×"
                        font.pixelSize: 13 * Theme.scaleFactor
                        color: Theme.muted
                        visible: !!searchInput.text
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: searchInput.text = ""
                        }
                    }
                }
            }
        }

        // 分割细线
        Rectangle {
            id: sepLine
            anchors.top: searchRow.bottom
            anchors.topMargin: 4 * Theme.scaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 12 * Theme.scaleFactor
            anchors.right: parent.right
            anchors.rightMargin: 12 * Theme.scaleFactor
            height: 1 * Theme.scaleFactor
            color: Theme.dark ? "#27272a" : "#f0f0f0"
        }

        // 3. 树形大纲列表
        Item {
            id: listContainer
            anchors.top: sepLine.bottom
            anchors.topMargin: 2 * Theme.scaleFactor
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            // 空状态提示
            Text {
                anchors.centerIn: parent
                text: searchQuery ? qsTr("No matching items") : qsTr("No outline available")
                font.pixelSize: 13 * Theme.scaleFactor
                color: Theme.muted
                visible: displayItems.length === 0
            }

            ListView {
                id: outlineList
                anchors.fill: parent
                clip: true
                model: displayItems

                delegate: Item {
                    width: outlineList.width
                    height: 28 * Theme.scaleFactor

                    readonly property bool isSelected: activeId === modelData.id

                    Rectangle {
                        id: rowCard
                        anchors.fill: parent
                        anchors.leftMargin: 6 * Theme.scaleFactor
                        anchors.rightMargin: 6 * Theme.scaleFactor
                        radius: 4 * Theme.scaleFactor
                        color: isSelected ? itemSelectBg : (rowMouse.containsMouse ? itemHoverBg : "transparent")

                        // 嵌套子级的轻量连接导引线（仅当处于子级 level > 0 且未选中时展现）
                        Item {
                            id: branchLines
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: (modelData.level * 16) * Theme.scaleFactor
                            visible: modelData.level > 0 && !isSelected

                            Rectangle {
                                x: (modelData.level * 16 - 8) * Theme.scaleFactor
                                y: 0
                                width: 1 * Theme.scaleFactor
                                height: modelData.isLast ? parent.height / 2 : parent.height
                                color: branchLineClr
                            }

                            Rectangle {
                                x: (modelData.level * 16 - 8) * Theme.scaleFactor
                                y: parent.height / 2
                                width: 8 * Theme.scaleFactor
                                height: 1 * Theme.scaleFactor
                                color: branchLineClr
                            }
                        }

                        // 展开/折叠三角指示符：使用旋转保证折叠与展开状态下箭头几何尺寸 100% 相同一致
                        Item {
                            id: arrowItem
                            z: 10
                            anchors.left: parent.left
                            anchors.leftMargin: (modelData.level * 16 + 2) * Theme.scaleFactor
                            anchors.verticalCenter: parent.verticalCenter
                            width: 20 * Theme.scaleFactor
                            height: 24 * Theme.scaleFactor

                            Text {
                                anchors.centerIn: parent
                                text: "∨"
                                font.pixelSize: 11 * Theme.scaleFactor
                                font.bold: true
                                color: isSelected ? "#ffffff" : chevronClr
                                visible: modelData.hasChildren
                                transformOrigin: Item.Center
                                rotation: modelData.expanded ? 0 : -90
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: modelData.hasChildren
                                cursorShape: Qt.PointingHandCursor
                                onClicked: toggleExpand(modelData.id)
                            }
                        }

                        // 章节标题文字
                        Text {
                            id: titleTextItem
                            anchors.left: arrowItem.right
                            anchors.leftMargin: 4 * Theme.scaleFactor
                            anchors.right: modelData.page ? pageText.left : parent.right
                            anchors.rightMargin: 8 * Theme.scaleFactor
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.title
                            elide: Text.ElideRight
                            font.pixelSize: 12.5 * Theme.scaleFactor
                            font.bold: isSelected || (modelData.level === 0)
                            color: isSelected ? itemSelectFg : (Theme.dark ? "#e4e4e7" : "#1f2328")
                        }

                        // 右侧对齐页码
                        Text {
                            id: pageText
                            anchors.right: parent.right
                            anchors.rightMargin: 8 * Theme.scaleFactor
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.page
                            font.pixelSize: 12 * Theme.scaleFactor
                            font.bold: isSelected
                            color: isSelected ? "#ffffff" : pageNumClr
                            visible: !!modelData.page
                        }

                        // 整行点击响应
                        MouseArea {
                            id: rowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (bridge) bridge.itemClicked(modelData.id, modelData.target);
                            }
                            onDoubleClicked: {
                                if (modelData.hasChildren) {
                                    toggleExpand(modelData.id);
                                }
                            }
                        }
                    }
                }
            }
        }

        // 4. 「···」弹出菜单（全部展开 / 全部折叠）
        Rectangle {
            id: popupMenu
            visible: menuOpen
            z: 100
            anchors.top: searchRow.bottom
            anchors.topMargin: 4 * Theme.scaleFactor
            anchors.right: searchRow.right
            width: 104 * Theme.scaleFactor
            height: menuColumn.implicitHeight + 8 * Theme.scaleFactor
            radius: 6 * Theme.scaleFactor
            color: menuBg
            border.width: 1 * Theme.scaleFactor
            border.color: menuBorder

            Column {
                id: menuColumn
                anchors.fill: parent
                anchors.margins: 4 * Theme.scaleFactor
                spacing: 2 * Theme.scaleFactor

                Repeater {
                    model: [
                        { text: qsTr("全部展开"), expand: true },
                        { text: qsTr("全部折叠"), expand: false }
                    ]

                    delegate: Rectangle {
                        width: menuColumn.width
                        height: 24 * Theme.scaleFactor
                        radius: 4 * Theme.scaleFactor
                        color: itemMouse.containsMouse ? itemHoverBg : "transparent"

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 8 * Theme.scaleFactor
                            text: modelData.text
                            font.pixelSize: 12 * Theme.scaleFactor
                            color: Theme.fg
                        }

                        MouseArea {
                            id: itemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: setAllExpanded(modelData.expand)
                        }
                    }
                }
            }
        }
    }
}
