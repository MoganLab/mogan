#!/bin/bash

# ================= 配置部分 =================
APP_NAME="mogan-stem"
BINARY_NAME="moganstem"
ARCH=$(dpkg --print-architecture)
INSTALL_PREFIX="/opt/$APP_NAME"

# 图标源路径 (相对于项目根目录)
ICON_SOURCE_REL="3rdparty/qwindowkitty/src/styles/app/stem.png"

# 尝试获取 VERSION
if [ -z "$VERSION" ]; then
    VERSION="2026.2.0"
else
    echo "✅ 检测到版本号: $VERSION"
fi

# 定位路径
if [ -L ${BASH_SOURCE-$0} ]; then
  FWDIR=$(dirname $(readlink "${BASH_SOURCE-$0}"))
else
  FWDIR=$(dirname "${BASH_SOURCE-$0}")
fi
APP_HOME="$(cd "${FWDIR}/../.."; pwd)"

APP_DIR="$APP_HOME/AppDir"
DEB_BUILD_DIR="$APP_HOME/deb_package"
DEPLOY_TOOL="linuxdeploy-x86_64.AppImage"
QT_PLUGIN="linuxdeploy-plugin-qt-x86_64.AppImage"

set -e

# ================= 1. 收集文件 =================
echo "📂 [1/6] 运行 xmake install 收集文件..."
cd "$APP_HOME"
rm -rf "$APP_DIR" "$DEB_BUILD_DIR"

# 安装二进制和资源
xmake install -o "$APP_DIR/usr" -y stem

if [ ! -f "$APP_DIR/usr/bin/$BINARY_NAME" ]; then
    echo "❌ 错误: 未找到二进制文件 $BINARY_NAME"
    exit 1
fi

# ================= 2. 处理图标 (应用程序图标和 MIME 类型图标) =================
echo "🎨 [2/6] 处理图标文件..."

# 2.1 应用程序图标
ICON_SRC="$APP_HOME/$ICON_SOURCE_REL"
# 只复制关键尺寸，减少不必要的复制
KEY_ICON_SIZES="16 32 48 64 128 256 512"
for size in $KEY_ICON_SIZES; do
    ICON_DEST_DIR="$APP_DIR/usr/share/icons/hicolor/${size}x${size}/apps"
    mkdir -p "$ICON_DEST_DIR"

    if [ -f "$ICON_SRC" ]; then
        echo "   -> 复制应用程序图标 ${size}x${size}"
        cp "$ICON_SRC" "$ICON_DEST_DIR/$APP_NAME.png"
    else
        echo "⚠️ 警告: 未找到应用程序图标源文件: $ICON_SRC"
        touch "$ICON_DEST_DIR/$APP_NAME.png"
    fi
done

# 2.2 MIME 类型图标 (用于 .tm, .tmu, .ts 文件)
echo "   -> 处理 MIME 类型图标..."
MIME_ICON_SRC_DIR="$APP_HOME/TeXmacs/misc/images"
MIME_ICON_NAME="texmacs-document"
MIME_PNG_SRC="$MIME_ICON_SRC_DIR/$MIME_ICON_NAME.png"
MIME_SVG_SRC="$MIME_ICON_SRC_DIR/$MIME_ICON_NAME.svg"

if [ -f "$MIME_PNG_SRC" ] || [ -f "$MIME_SVG_SRC" ]; then
    echo "   -> 找到 MIME 类型图标源文件"

    # 复制 PNG 图标到关键尺寸
    if [ -f "$MIME_PNG_SRC" ]; then
        for size in $KEY_ICON_SIZES; do
            MIME_ICON_DEST_DIR="$APP_DIR/usr/share/icons/hicolor/${size}x${size}/mimetypes"
            mkdir -p "$MIME_ICON_DEST_DIR"
            cp "$MIME_PNG_SRC" "$MIME_ICON_DEST_DIR/$MIME_ICON_NAME.png"
        done
    fi

    # 复制 SVG 图标
    if [ -f "$MIME_SVG_SRC" ]; then
        MIME_ICON_DEST_DIR="$APP_DIR/usr/share/icons/hicolor/scalable/mimetypes"
        mkdir -p "$MIME_ICON_DEST_DIR"
        cp "$MIME_SVG_SRC" "$MIME_ICON_DEST_DIR/$MIME_ICON_NAME.svg"
    fi

    echo "   -> MIME 类型图标复制完成"
else
    echo "⚠️ 警告: 未找到 MIME 类型图标源文件: $MIME_ICON_SRC_DIR/$MIME_ICON_NAME.{png,svg}"
fi

# ================= 3. 确保 .desktop 文件存在并正确 =================
DESKTOP_PATH="$APP_DIR/usr/share/applications/$APP_NAME.desktop"
if [ ! -f "$DESKTOP_PATH" ]; then
    echo "📄 [3/6] 生成 .desktop 文件..."
    mkdir -p "$(dirname "$DESKTOP_PATH")"
    cat > "$DESKTOP_PATH" <<EOF
[Desktop Entry]
Version=$VERSION
Type=Application
Name=Mogan STEM
GenericName=Mogan STEM
Comment=Scientific Editor
MimeType=text/x-texmacs.doc;text/x-texmacs.sty;text/plain;text/x-tex;
Exec=$BINARY_NAME
Icon=$APP_NAME
Terminal=false
Categories=Education;Science;Math;
X-KDE-Priority=TopLevel
EOF
else
    # 强制修正 Icon 字段，确保它使用我们刚才复制进去的图标名
    echo "   -> 更新现有 .desktop 文件的图标设置..."
    sed -i "s|^Icon=.*|Icon=$APP_NAME|" "$DESKTOP_PATH"
    # 确保Exec路径正确
    sed -i "s|^Exec=.*|Exec=$BINARY_NAME|" "$DESKTOP_PATH"
fi


# ================= 4. 准备工具 =================
echo "🛠️ [4/6] 准备 LinuxDeploy..."
if [ ! -f "$DEPLOY_TOOL" ]; then
    wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/$DEPLOY_TOOL"
    chmod +x "$DEPLOY_TOOL"
fi
if [ ! -f "$QT_PLUGIN" ]; then
    wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/$QT_PLUGIN"
    chmod +x "$QT_PLUGIN"
fi

# ================= 5. 打包依赖 (Bundle) =================
echo "🔍 [5/6] 注入 Qt 依赖..."
XMAKE_QMAKE=$(find ~/.xmake/packages -type f -name qmake 2>/dev/null | grep "qt" | head -n 1)
if [ -n "$XMAKE_QMAKE" ]; then
    export QMAKE="$XMAKE_QMAKE"
    export PATH="$(dirname "$XMAKE_QMAKE"):$PATH"
fi

# -------------------------------------------------------------
# 手动导入输入法插件 (Fix for Chinese Input Method)
# -------------------------------------------------------------
echo "🔧 [Manual Import] 正在导入 Fcitx5/中文输入法支持..."

# 1. 定义我们要在 AppDir (安装包) 里存放插件的位置
#    Qt 程序默认去 plugins/platforminputcontexts 找输入法
DEST_PLUGIN_DIR="$APP_DIR/usr/plugins/platforminputcontexts"
mkdir -p "$DEST_PLUGIN_DIR"

# 2. 定义系统源路径 
SRC_PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/qt6/plugins/platforminputcontexts"

# 3. 执行复制
if [ -d "$SRC_PLUGIN_DIR" ]; then
    echo "   -> 发现系统插件目录: $SRC_PLUGIN_DIR"
    # 复制该目录下所有 .so 文件到包内的插件目录
    cp -v "$SRC_PLUGIN_DIR/"*.so "$DEST_PLUGIN_DIR/" 2>/dev/null || true
    echo "   -> 复制完成。"
else
    echo "⚠️ 警告: 未在系统中找到 $SRC_PLUGIN_DIR"
    echo "   请确保构建环境安装了 'fcitx5-frontend-qt6' 或 'libqt6gui6'。"
fi
# -------------------------------------------------------------

# 运行 linuxdeploy
# 它会扫描我们刚才复制进去的 .so 文件，并把它们依赖的 fcitx 库也打包进去
./"$DEPLOY_TOOL" --appdir "$APP_DIR" --plugin qt --executable "$APP_DIR/usr/bin/$BINARY_NAME" --icon-file "$ICON_SRC"

# ================= 6. 构建 /opt 包结构 =================
echo "📦 [6/6] 组装并生成 Deb..."
mkdir -p "$DEB_BUILD_DIR/DEBIAN"
mkdir -p "$DEB_BUILD_DIR$INSTALL_PREFIX"

# 移动内容到 /opt/mogan-stem
cp -r "$APP_DIR/usr/"* "$DEB_BUILD_DIR$INSTALL_PREFIX/"

# 移动 .desktop 文件到 /usr/share/applications/
DESKTOP_SRC=$(find "$DEB_BUILD_DIR$INSTALL_PREFIX/share/applications" -name "*.desktop" | head -n 1)
if [ -f "$DESKTOP_SRC" ]; then
    mkdir -p "$DEB_BUILD_DIR/usr/share/applications"
    DESKTOP_DEST="$DEB_BUILD_DIR/usr/share/applications/$(basename "$DESKTOP_SRC")"
    cp "$DESKTOP_SRC" "$DESKTOP_DEST"
    # 修正 Exec 路径为绝对路径
    sed -i "s|^Exec=.*|Exec=$INSTALL_PREFIX/bin/$BINARY_NAME|" "$DESKTOP_DEST"
    # 删除原始位置的文件
    rm "$DESKTOP_SRC"
fi

# 移动图标文件到 /usr/share/icons/
ICON_SRC_DIR="$DEB_BUILD_DIR$INSTALL_PREFIX/share/icons"
if [ -d "$ICON_SRC_DIR" ]; then
    mkdir -p "$DEB_BUILD_DIR/usr/share/icons"
    # 只复制hicolor目录，保留MIME图标
    if [ -d "$ICON_SRC_DIR/hicolor" ]; then
        cp -r "$ICON_SRC_DIR/hicolor" "$DEB_BUILD_DIR/usr/share/icons/"
    fi
    rm -rf "$ICON_SRC_DIR"
fi

# 移动 MIME 文件到 /usr/share/mime/
MIME_SRC_DIR="$DEB_BUILD_DIR$INSTALL_PREFIX/share/mime"
if [ -d "$MIME_SRC_DIR" ]; then
    mkdir -p "$DEB_BUILD_DIR/usr/share/mime"
    cp -r "$MIME_SRC_DIR/"* "$DEB_BUILD_DIR/usr/share/mime/"
    rm -rf "$MIME_SRC_DIR"
else
    # 如果 xmake install 没有安装 MIME 文件，手动复制
    echo "   -> 手动复制 MIME 类型定义文件..."
    MIME_SRC_FILE="$APP_HOME/TeXmacs/misc/mime/texmacs.xml"
    if [ -f "$MIME_SRC_FILE" ]; then
        mkdir -p "$DEB_BUILD_DIR/usr/share/mime/packages"
        cp "$MIME_SRC_FILE" "$DEB_BUILD_DIR/usr/share/mime/packages/"
        echo "   -> 已复制 MIME 类型定义文件"
    else
        echo "⚠️ 警告: 未找到 MIME 类型定义文件: $MIME_SRC_FILE"
    fi
fi

# 生成 Control
INSTALLED_SIZE=$(du -s "$DEB_BUILD_DIR" | cut -f1)
cat > "$DEB_BUILD_DIR/DEBIAN/control" <<EOF
Package: $APP_NAME
Version: $VERSION
Architecture: $ARCH
Maintainer: Mogan Team <dev@mogan.app>
Installed-Size: $INSTALLED_SIZE
Section: science
Priority: optional
Description: Mogan Stem
 Scientific editor powered by Mogan.
 Installed in $INSTALL_PREFIX.
EOF

# 生成 post-install 脚本，更新 MIME 数据库和创建命令行别名
cat > "$DEB_BUILD_DIR/DEBIAN/postinst" <<EOF
#!/bin/bash
set -e

echo "Updating MIME type database..."
if command -v update-mime-database >/dev/null 2>&1; then
    if ! update-mime-database /usr/share/mime; then
        echo "Error: Failed to update MIME database" >&2
    else
        echo "MIME database updated"
    fi
else
    echo "Warning: update-mime-database command not found"
fi

echo "Updating desktop icon cache..."
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    if ! gtk-update-icon-cache -f /usr/share/icons/hicolor; then
        echo "Error: Failed to update GTK icon cache" >&2
    else
        echo "GTK icon cache updated"
    fi
else
    echo "Warning: gtk-update-icon-cache command not found"
fi

if command -v kbuildsycoca5 >/dev/null 2>&1; then
    if ! kbuildsycoca5 --noincremental; then
        echo "Error: Failed to update KDE system configuration cache" >&2
    else
        echo "KDE system configuration cache updated"
    fi
elif command -v kbuildsycoca6 >/dev/null 2>&1; then
    if ! kbuildsycoca6 --noincremental; then
        echo "Error: Failed to update KDE system configuration cache" >&2
    else
        echo "KDE system configuration cache updated"
    fi
fi

echo "Creating command-line alias..."
# Create command-line alias so users can run mogan-stem directly
BINARY_PATH="/opt/mogan-stem/bin/moganstem"
ALIAS_NAME="mogan-stem"
TARGET_LINK="/usr/local/bin/\$ALIAS_NAME"

if [ -f "\$BINARY_PATH" ] && [ -x "\$BINARY_PATH" ]; then
    # Create symbolic link in /usr/local/bin
    if [ -d "/usr/local/bin" ]; then
        if [ -L "\$TARGET_LINK" ]; then
            # 如果是符号链接，安全覆盖
            ln -sf "\$BINARY_PATH" "\$TARGET_LINK"
            echo "   -> Updated existing symlink: mogan-stem"
            echo "      (symlink: /usr/local/bin/mogan-stem -> /opt/mogan-stem/bin/moganstem)"
        elif [ -e "\$TARGET_LINK" ]; then
            # 如果是普通文件，警告用户
            echo "   -> Warning: \$TARGET_LINK already exists and is not a symlink"
            echo "      Skipping alias creation to avoid overwriting user file"
            echo "      You can manually create symlink: ln -s /opt/mogan-stem/bin/moganstem /usr/local/bin/mogan-stem"
        else
            # 文件不存在，安全创建
            ln -s "\$BINARY_PATH" "\$TARGET_LINK"
            echo "   -> Created command-line alias: mogan-stem"
            echo "      (symlink: /usr/local/bin/mogan-stem -> /opt/mogan-stem/bin/moganstem)"
        fi
    else
        echo "   -> Warning: /usr/local/bin directory does not exist"
    fi
else
    echo "   -> Error: Binary not found or not executable: \$BINARY_PATH" >&2
fi

echo "Installation complete! You can now:"
echo "  1. Run from terminal: mogan-stem (if alias was created)"
echo "  2. Launch from application menu: Mogan Stem"
echo "  3. Double-click .tm, .tmu, .ts files to open with Mogan Stem"
EOF

chmod 755 "$DEB_BUILD_DIR/DEBIAN/postinst"

# 生成 pre-remove 脚本，清理符号链接
cat > "$DEB_BUILD_DIR/DEBIAN/prerm" <<EOF
#!/bin/bash
set -e

echo "Cleaning up command-line alias..."
ALIAS_NAME="mogan-stem"

# Remove symbolic link from /usr/local/bin
if [ -L "/usr/local/bin/\$ALIAS_NAME" ]; then
    rm -f "/usr/local/bin/\$ALIAS_NAME"
    echo "   -> Removed command-line alias: mogan-stem"
fi
EOF

chmod 755 "$DEB_BUILD_DIR/DEBIAN/prerm"

OUTPUT_DEB="${APP_HOME}/../${APP_NAME}_${VERSION}_${ARCH}.deb"
dpkg-deb --build "$DEB_BUILD_DIR" "$OUTPUT_DEB"

# 设置适当的权限，避免安装时出现 _apt 用户权限警告
chmod 644 "$OUTPUT_DEB"

echo "✅ 打包完成: $OUTPUT_DEB"
echo "💡 安装命令: sudo dpkg -i \"$OUTPUT_DEB\""
