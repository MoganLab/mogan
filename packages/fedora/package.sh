#!/bin/bash

# ================= 配置部分 =================
APP_NAME="mogan-stem"
BINARY_NAME="moganstem"
ARCH=$(uname -m)
INSTALL_PREFIX="/opt/$APP_NAME"

# 图标源路径 (相对于项目根目录)
ICON_SOURCE_REL="3rdparty/qwindowkitty/src/styles/app/stem.png"

# 尝试获取 VERSION
if [ -z "$VERSION" ]; then
    VERSION="2026.2.3"
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
RPM_BUILD_DIR="$APP_HOME/rpmbuild"
DEPLOY_TOOL="linuxdeploy-x86_64.AppImage"
QT_PLUGIN="linuxdeploy-plugin-qt-x86_64.AppImage"

set -e

# ================= 1. 收集文件 =================
echo "📂 [1/6] 运行 xmake install 收集文件..."
cd "$APP_HOME"
rm -rf "$APP_DIR" "$RPM_BUILD_DIR"

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
KEY_ICON_SIZES="16 32 48 64 128 256 512"
for size in $KEY_ICON_SIZES; do
    ICON_DEST_DIR="$APP_DIR/usr/share/icons/hicolor/${size}x${size}/apps"
    mkdir -p "$ICON_DEST_DIR"

    if [ -f "$ICON_SRC" ]; then
        echo "   -> 复制应用程序图标 ${size}x${size}"
        cp "$ICON_SRC" "$ICON_DEST_DIR/$APP_NAME.png"
    else
        echo "⚠️  警告: 未找到应用程序图标源文件: $ICON_SRC"
        touch "$ICON_DEST_DIR/$APP_NAME.png"
    fi
done

# 2.2 MIME 类型图标
echo "   -> 处理 MIME 类型图标..."
MIME_ICON_SRC_DIR="$APP_HOME/TeXmacs/misc/images"
MIME_ICON_NAME="texmacs-document"
MIME_PNG_SRC="$MIME_ICON_SRC_DIR/$MIME_ICON_NAME.png"
MIME_SVG_SRC="$MIME_ICON_SRC_DIR/$MIME_ICON_NAME.svg"

if [ -f "$MIME_PNG_SRC" ] || [ -f "$MIME_SVG_SRC" ]; then
    echo "   -> 找到 MIME 类型图标源文件"
    if [ -f "$MIME_PNG_SRC" ]; then
        for size in $KEY_ICON_SIZES; do
            MIME_ICON_DEST_DIR="$APP_DIR/usr/share/icons/hicolor/${size}x${size}/mimetypes"
            mkdir -p "$MIME_ICON_DEST_DIR"
            cp "$MIME_PNG_SRC" "$MIME_ICON_DEST_DIR/$MIME_ICON_NAME.png"
        done
    fi
    if [ -f "$MIME_SVG_SRC" ]; then
        MIME_ICON_DEST_DIR="$APP_DIR/usr/share/icons/hicolor/scalable/mimetypes"
        mkdir -p "$MIME_ICON_DEST_DIR"
        cp "$MIME_SVG_SRC" "$MIME_ICON_DEST_DIR/$MIME_ICON_NAME.svg"
    fi
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
    sed -i "s|^Icon=.*|Icon=$APP_NAME|" "$DESKTOP_PATH"
    sed -i "s|^Exec=.*|Exec=$BINARY_NAME|" "$DESKTOP_PATH"
fi


# ================= 4. 准备工具 =================
echo "🛠️  [4/6] 准备 LinuxDeploy..."
if [ ! -f "$DEPLOY_TOOL" ]; then
    curl -fsSL -o "$DEPLOY_TOOL" "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/$DEPLOY_TOOL"
    chmod +x "$DEPLOY_TOOL"
fi
if [ ! -f "$QT_PLUGIN" ]; then
    curl -fsSL -o "$QT_PLUGIN" "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/$QT_PLUGIN"
    chmod +x "$QT_PLUGIN"
fi

# ================= 5. 打包依赖 (Bundle) =================
echo "🔍 [5/6] 注入 Qt 依赖..."
if [ -n "$XMAKE_GLOBALDIR" ]; then
    XMAKE_QMAKE=$(find "$XMAKE_GLOBALDIR/.xmake/packages" -type f -name qmake 2>/dev/null | grep "qt" | head -n 1)
fi
if [ -z "$XMAKE_QMAKE" ]; then
    XMAKE_QMAKE=$(find ~/.xmake/packages -type f -name qmake 2>/dev/null | grep "qt" | head -n 1)
fi
if [ -n "$XMAKE_QMAKE" ]; then
    export QMAKE="$XMAKE_QMAKE"
    export PATH="$(dirname "$XMAKE_QMAKE"):$PATH"
fi

# 导入 Qt 插件（从 xmake 的 Qt，确保版本兼容）
echo "🔧 [Manual Import] 正在导入 Qt 插件..."

if [ -n "$XMAKE_QMAKE" ]; then
    XMAKE_QT_DIR=$(dirname "$XMAKE_QMAKE")

    # 复制平台插件
    DEST_PLATFORM_DIR="$APP_DIR/usr/plugins/platforms"
    mkdir -p "$DEST_PLATFORM_DIR"
    SRC_PLATFORM_DIR="$XMAKE_QT_DIR/../plugins/platforms"
    if [ -d "$SRC_PLATFORM_DIR" ]; then
        echo "   -> 复制平台插件..."
        cp -v "$SRC_PLATFORM_DIR/"*.so "$DEST_PLATFORM_DIR/" 2>/dev/null || true
    fi

    # 复制 TLS 插件
    DEST_TLS_DIR="$APP_DIR/usr/plugins/tls"
    mkdir -p "$DEST_TLS_DIR"
    SRC_TLS_DIR="$XMAKE_QT_DIR/../plugins/tls"
    if [ -d "$SRC_TLS_DIR" ]; then
        echo "   -> 复制 TLS 插件..."
        cp -v "$SRC_TLS_DIR/"*.so "$DEST_TLS_DIR/" 2>/dev/null || true
    fi

    # 复制其他必要插件
    for plugin_dir in imageformats iconengines; do
        SRC_DIR="$XMAKE_QT_DIR/../plugins/$plugin_dir"
        DEST_DIR="$APP_DIR/usr/plugins/$plugin_dir"
        if [ -d "$SRC_DIR" ]; then
            echo "   -> 复制 $plugin_dir 插件..."
            mkdir -p "$DEST_DIR"
            cp -v "$SRC_DIR/"*.so "$DEST_DIR/" 2>/dev/null || true
        fi
    done
else
    echo "⚠️  警告: 未找到 xmake Qt"
fi

# 导入输入法插件（从 xmake Qt，确保版本兼容）
echo "🔧 [Manual Import] 正在导入输入法插件（IBus/Compose）..."
DEST_PLUGIN_DIR="$APP_DIR/usr/plugins/platforminputcontexts"
mkdir -p "$DEST_PLUGIN_DIR"

if [ -n "$XMAKE_QMAKE" ]; then
    XMAKE_QT_DIR=$(dirname "$XMAKE_QMAKE")
    SRC_INPUT_DIR="$XMAKE_QT_DIR/../plugins/platforminputcontexts"
    if [ -d "$SRC_INPUT_DIR" ]; then
        echo "   -> 发现 xmake Qt 输入法插件目录: $SRC_INPUT_DIR"
        cp -v "$SRC_INPUT_DIR/"*.so "$DEST_PLUGIN_DIR/" 2>/dev/null || true
        echo "   -> 输入法插件复制完成"
    else
        echo "   -> 警告: xmake Qt 中未找到输入法插件"
    fi
fi

# 复制 IBus 相关的系统库（这些不是 Qt 插件，是系统库）
echo "   -> 复制 IBus 系统库..."
for lib_dir in /usr/lib64 /usr/lib; do
    if [ -d "$lib_dir" ]; then
        for lib in libibus-*.so*; do
            if [ -f "$lib_dir/$lib" ]; then
                cp -v "$lib_dir/$lib" "$APP_DIR/usr/lib/" 2>/dev/null || true
            fi
        done
    fi
done

# 运行 linuxdeploy
./"$DEPLOY_TOOL" --appdir "$APP_DIR" --plugin qt --executable "$APP_DIR/usr/bin/$BINARY_NAME" --icon-file "$ICON_SRC" 2>&1 || true

# 复制 xcb-cursor 库（Qt 6.5+ 需要）
echo "🔧 复制 xcb-cursor 库..."
if [ -f /usr/lib64/libxcb-cursor.so.0 ]; then
    cp -v /usr/lib64/libxcb-cursor.so.0* "$APP_DIR/usr/lib/" 2>/dev/null || true
    echo "   -> xcb-cursor 复制完成"
fi

# 复制 Qt6XcbQpa 和其他 Qt 库（确保版本与应用程序 Qt 一致）
echo "🔧 复制 xmake Qt 库（避免版本冲突）..."
if [ -n "$XMAKE_QMAKE" ]; then
    XMAKE_QT_DIR=$(dirname "$XMAKE_QMAKE")
    # 复制所有 xmake Qt 库
    for lib in "$XMAKE_QT_DIR"/../lib/libQt6*.so.6; do
        if [ -f "$lib" ]; then
            cp -v "$lib" "$APP_DIR/usr/lib/" 2>/dev/null || true
        fi
    done
    # 复制符号链接
    for lib in "$XMAKE_QT_DIR"/../lib/libQt6*.so; do
        if [ -L "$lib" ]; then
            cp -P "$lib" "$APP_DIR/usr/lib/" 2>/dev/null || true
        fi
    done
    echo "   -> Qt 库复制完成"
fi

# 使用系统自带的 strip 工具手动瘦身
echo "🔧 使用系统 strip 工具优化库文件大小..."
find "$APP_DIR/usr/lib" -type f -name "*.so*" -exec strip --strip-unneeded {} \; 2>/dev/null || true
echo "   -> 优化完成"

# 修复 rpath，清除硬编码的构建路径
echo "🔧 修复 ELF 文件的 rpath..."
if command -v patchelf >/dev/null 2>&1; then
    # 清除主程序的 rpath
    patchelf --set-rpath '$ORIGIN/../lib' "$APP_DIR/usr/bin/$BINARY_NAME" 2>/dev/null || true
    # 清除所有 .so 文件的 rpath
    find "$APP_DIR/usr/lib" -type f -name "*.so*" -exec patchelf --set-rpath '$ORIGIN' {} \; 2>/dev/null || true
    echo "   -> rpath 修复完成"
else
    echo "⚠️  patchelf 未安装，跳过 rpath 修复"
fi

# ================= 6. 构建 RPM 包 =================
echo "📦 [6/6] 组装并生成 RPM..."

# 创建 RPM 构建目录结构
mkdir -p "$RPM_BUILD_DIR"/{BUILD,RPMS,SOURCES,SPECS,SRPMS,BUILDROOT}

# 创建源码 tarball
TARBALL_NAME="${APP_NAME}-${VERSION}.tar.gz"
TARBALL_DIR="${APP_NAME}-${VERSION}"

# 创建临时目录准备源码
cd /tmp
rm -rf "$TARBALL_DIR"
mkdir -p "$TARBALL_DIR/usr"

# 复制准备好的文件到源码目录
cp -r "$APP_DIR/usr/"* "$TARBALL_DIR/usr/"
# 复制 plugins 目录（由 linuxdeploy 生成）
if [ -d "$APP_DIR/usr/plugins" ]; then
    cp -r "$APP_DIR/usr/plugins" "$TARBALL_DIR/usr/"
fi

# 打包源码
tar czf "$RPM_BUILD_DIR/SOURCES/$TARBALL_NAME" "$TARBALL_DIR"
rm -rf "$TARBALL_DIR"

cd "$APP_HOME"

# 生成 SPEC 文件 - 使用单引号禁止变量扩展，然后手动替换
SPEC_FILE="$RPM_BUILD_DIR/SPECS/$APP_NAME.spec"

cat > "$SPEC_FILE" << 'SPECEOF'
# 禁用 debuginfo 包生成
%define debug_package %{nil}
%define _build_id_links none
# 禁用自动依赖检测（Qt 私有 API 依赖系统没有）
AutoReq: no
AutoProv: no

Name:           APP_NAME_PLACEHOLDER
Version:        VERSION_PLACEHOLDER
Release:        1%{?dist}
Summary:        A structured editor for science and technology
License:        GPLv3+
URL:            https://mogan.app
Source0:        %{name}-%{version}.tar.gz
BuildArch:      ARCH_PLACEHOLDER
Group:          Applications/Editors

%description
Mogan Research is a fork of GNU TeXmacs using S7 Scheme and Qt 6 with massive
online TeXmacs documents provided by Xmacs Planet and TMML wiki.
The software includes a text editor with support for mathematical formulas,
a small technical picture editor and a tool for making presentations from
a laptop. Moreover, Mogan can be used as an interface for many external
systems for computer algebra, numerical analysis, statistics, etc.
New presentation styles can be written by the user and new features can be
added to the editor using the Scheme extension language. A native spreadsheet
and tools for collaborative authoring are planned for later.
Mogan runs on all major Unix platforms and Windows. Documents can be
saved in TeXmacs, Xml or Scheme format and printed as Postscript or
Pdf files. Converters exist for TeX/LaTeX and Html/Mathml.

%prep
%setup -q

%build
# 无需构建，二进制已准备好

%install
# 创建目标目录
mkdir -p %{buildroot}INSTALL_PREFIX_PLACEHOLDER
mkdir -p %{buildroot}/usr/share/applications
mkdir -p %{buildroot}/usr/share/icons
mkdir -p %{buildroot}/usr/share/mime/packages
mkdir -p %{buildroot}/usr/local/bin

# 复制程序文件到 /opt/mogan-stem
cp -r usr/* %{buildroot}INSTALL_PREFIX_PLACEHOLDER/

# 复制 .desktop 文件
if [ -f "%{buildroot}INSTALL_PREFIX_PLACEHOLDER/share/applications/APP_NAME_PLACEHOLDER.desktop" ]; then
    cp "%{buildroot}INSTALL_PREFIX_PLACEHOLDER/share/applications/APP_NAME_PLACEHOLDER.desktop" %{buildroot}/usr/share/applications/
    sed -i "s|^Exec=.*|Exec=INSTALL_PREFIX_PLACEHOLDER/bin/BINARY_NAME_PLACEHOLDER|" %{buildroot}/usr/share/applications/APP_NAME_PLACEHOLDER.desktop
    rm "%{buildroot}INSTALL_PREFIX_PLACEHOLDER/share/applications/APP_NAME_PLACEHOLDER.desktop"
fi

# 复制图标文件
if [ -d "%{buildroot}INSTALL_PREFIX_PLACEHOLDER/share/icons/hicolor" ]; then
    cp -r "%{buildroot}INSTALL_PREFIX_PLACEHOLDER/share/icons/hicolor" %{buildroot}/usr/share/icons/
    rm -rf "%{buildroot}INSTALL_PREFIX_PLACEHOLDER/share/icons"
fi

# 复制 MIME 文件
if [ -d "%{buildroot}INSTALL_PREFIX_PLACEHOLDER/share/mime" ]; then
    cp -r "%{buildroot}INSTALL_PREFIX_PLACEHOLDER/share/mime/packages" %{buildroot}/usr/share/mime/ 2>/dev/null || true
    rm -rf "%{buildroot}INSTALL_PREFIX_PLACEHOLDER/share/mime"
fi

# 如果没有 mime 文件，手动复制
if [ ! -f "%{buildroot}/usr/share/mime/packages/texmacs.xml" ]; then
    mkdir -p %{buildroot}/usr/share/mime/packages
    if [ -f "APP_HOME_PLACEHOLDER/TeXmacs/misc/mime/texmacs.xml" ]; then
        cp "APP_HOME_PLACEHOLDER/TeXmacs/misc/mime/texmacs.xml" %{buildroot}/usr/share/mime/packages/
    fi
fi

# 创建 qt.conf 告诉 Qt 插件位置
cat > %{buildroot}INSTALL_PREFIX_PLACEHOLDER/bin/qt.conf << 'QTEOF'
[Paths]
Prefix = INSTALL_PREFIX_PLACEHOLDER
Plugins = INSTALL_PREFIX_PLACEHOLDER/plugins
QTEOF

# 创建 wrapper 脚本（设置 Qt 插件路径和库路径）
cat > %{buildroot}/usr/local/bin/APP_NAME_PLACEHOLDER << 'WRAPPEREOF'
#!/usr/bin/env bash
# Mogan STEM Launcher
export QT_PLUGIN_PATH=INSTALL_PREFIX_PLACEHOLDER/plugins:$QT_PLUGIN_PATH
export LD_LIBRARY_PATH=INSTALL_PREFIX_PLACEHOLDER/lib:$LD_LIBRARY_PATH
exec INSTALL_PREFIX_PLACEHOLDER/bin/BINARY_NAME_PLACEHOLDER "$@"
WRAPPEREOF
chmod +x %{buildroot}/usr/local/bin/APP_NAME_PLACEHOLDER

%files
%defattr(-,root,root,-)
INSTALL_PREFIX_PLACEHOLDER
INSTALL_PREFIX_PLACEHOLDER/bin/qt.conf
/usr/share/applications/APP_NAME_PLACEHOLDER.desktop
/usr/share/icons/hicolor
/usr/share/mime/packages
%attr(755,root,root) /usr/local/bin/APP_NAME_PLACEHOLDER

%post
# 更新 MIME 数据库
echo "Updating MIME type database..."
if command -v update-mime-database > /dev/null 2>&1; then
    update-mime-database /usr/share/mime || true
fi

# 更新图标缓存
echo "Updating icon cache..."
if command -v gtk-update-icon-cache > /dev/null 2>&1; then
    gtk-update-icon-cache -f /usr/share/icons/hicolor || true
fi

# 更新 KDE 缓存
if command -v kbuildsycoca5 > /dev/null 2>&1; then
    kbuildsycoca5 --noincremental || true
elif command -v kbuildsycoca6 > /dev/null 2>&1; then
    kbuildsycoca6 --noincremental || true
fi

echo "Installation complete! You can now:"
echo "  1. Run from terminal: APP_NAME_PLACEHOLDER"
echo "  2. Launch from application menu: Mogan STEM"

%preun
# 卸载前的清理

%postun
# 卸载后更新 MIME 数据库
if [ $1 -eq 0 ]; then
    if command -v update-mime-database > /dev/null 2>&1; then
        update-mime-database /usr/share/mime || true
    fi
    if command -v gtk-update-icon-cache > /dev/null 2>&1; then
        gtk-update-icon-cache -f /usr/share/icons/hicolor || true
    fi
fi

%changelog
* CHANGLOG_DATE Darcy Shen <da@liii.pro> - VERSION_PLACEHOLDER-1
- Initial RPM package
SPECEOF

# 替换占位符
sed -i "s|APP_NAME_PLACEHOLDER|$APP_NAME|g" "$SPEC_FILE"
sed -i "s|VERSION_PLACEHOLDER|$VERSION|g" "$SPEC_FILE"
sed -i "s|ARCH_PLACEHOLDER|$ARCH|g" "$SPEC_FILE"
sed -i "s|INSTALL_PREFIX_PLACEHOLDER|$INSTALL_PREFIX|g" "$SPEC_FILE"
sed -i "s|BINARY_NAME_PLACEHOLDER|$BINARY_NAME|g" "$SPEC_FILE"
sed -i "s|APP_HOME_PLACEHOLDER|$APP_HOME|g" "$SPEC_FILE"
sed -i "s|CHANGLOG_DATE|$(LC_ALL=C date "+%a %b %d %Y")|g" "$SPEC_FILE"

# 使用 rpmbuild 构建 RPM
cd "$APP_HOME"
rpmbuild --define "_topdir $RPM_BUILD_DIR" -bb "$SPEC_FILE"

# 查找生成的 RPM 包（处理 %{?dist} 后缀如 .fc43）
OUTPUT_RPM=$(find "$RPM_BUILD_DIR/RPMS" -name "${APP_NAME}-${VERSION}-1.*.${ARCH}.rpm" -type f | head -n1)

if [ -f "$OUTPUT_RPM" ]; then
    echo "✅ 打包完成: $OUTPUT_RPM"
    echo "💡 安装命令: sudo rpm -i \"$OUTPUT_RPM\""
else
    echo "❌ 错误: RPM 包未生成"
    exit 1
fi
