#!/bin/bash
# ============================================================
# CentOS 7.9 一键打包 COM_and_RJ45_TEST 为 AppImage
# 用法: chmod +x pack_centos7.sh && ./pack_centos7.sh
# 前提: 已用 qmake && make 编译出 COM_and_RJ45_TEST 二进制
# ============================================================
set -e

APP_NAME="COM_and_RJ45_TEST"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APPIMAGE_FILE="${APP_NAME}-x86_64.AppImage"

echo "========================================"
echo " CentOS 7.9 打包 $APP_NAME 为 AppImage"
echo "========================================"

# ---- 1. 检查编译产物 ----
echo "[1/7] 检查编译产物..."
if [ ! -f "$SCRIPT_DIR/$APP_NAME" ]; then
    echo "❌ 错误: 未找到 $APP_NAME"
    echo "   请先在 CentOS 7.9 上编译: qmake $APP_NAME.pro && make"
    exit 1
fi
echo "    ✓ 找到 $APP_NAME"

# ---- 2. 检查 Qt 路径 ----
echo "[2/7] 检查 Qt 安装路径..."
QMAKE_PATH=$(command -v qmake-qt5 || command -v qmake || echo "")
if [ -z "$QMAKE_PATH" ]; then
    echo "❌ 错误: 未找到 qmake，请安装: sudo yum install qt5-qtbase-devel qt5-qtserialport-devel"
    exit 1
fi

# 获取 Qt 库路径
QT_LIB_DIR=$(qmake-qt5 -query QT_INSTALL_LIBS 2>/dev/null || qmake -query QT_INSTALL_LIBS 2>/dev/null)
QT_PLUGIN_DIR=$(qmake-qt5 -query QT_INSTALL_PLUGINS 2>/dev/null || qmake -query QT_INSTALL_PLUGINS 2>/dev/null)
QT_QML_DIR=$(qmake-qt5 -query QT_INSTALL_QML 2>/dev/null || qmake -query QT_INSTALL_QML 2>/dev/null)
echo "    Qt 库路径: $QT_LIB_DIR"
echo "    Qt 插件路径: $QT_PLUGIN_DIR"

# ---- 3. 准备 AppDir ----
echo "[3/7] 准备 AppDir 目录..."
APPDIR="$SCRIPT_DIR/AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/share/icons/hicolor/48x48/apps"

# 复制可执行文件
cp "$SCRIPT_DIR/$APP_NAME" "$APPDIR/usr/bin/"

# ---- 4. 复制 CAN 动态库 ----
echo "[4/7] 复制 CAN 动态库..."
if [ -f "$SCRIPT_DIR/cx/libcontrolcan.so" ]; then
    cp "$SCRIPT_DIR/cx/libcontrolcan.so" "$APPDIR/usr/lib/"
    echo "    ✓ libcontrolcan.so"
fi
if [ -f "$SCRIPT_DIR/zy/libusbcan.so" ]; then
    cp "$SCRIPT_DIR/zy/libusbcan.so" "$APPDIR/usr/lib/"
    echo "    ✓ libusbcan.so"
fi
if [ -f "$SCRIPT_DIR/libucan2/libucan2.so" ]; then
    cp "$SCRIPT_DIR/libucan2/libucan2.so" "$APPDIR/usr/lib/"
    echo "    ✓ libucan2.so"
fi

# ---- 5. 复制 Qt 依赖库 ----
echo "[5/7] 复制 Qt 运行时库..."

copy_qt_lib() {
    local libname="$1"
    local src="$QT_LIB_DIR/lib$libname.so"
    if [ -f "$src" ]; then
        # 解析实际的 .so 文件（跟随符号链接找到真实文件）
        local real_file=$(readlink -f "$src" 2>/dev/null || echo "$src")
        cp -L "$real_file" "$APPDIR/usr/lib/"
        echo "    ✓ $libname"
    else
        echo "    ⚠ 未找到 $libname"
    fi
}

# Qt 核心库
copy_qt_lib "Qt5Core.so.5"
copy_qt_lib "Qt5Gui.so.5"
copy_qt_lib "Qt5Widgets.so.5"
copy_qt_lib "Qt5SerialPort.so.5"
copy_qt_lib "Qt5Svg.so.5"
copy_qt_lib "Qt5Network.so.5"
copy_qt_lib "Qt5DBus.so.5"

# Qt 平台插件
echo "   复制 Qt 平台插件..."
mkdir -p "$APPDIR/usr/plugins/platforms"
if [ -d "$QT_PLUGIN_DIR/platforms" ]; then
    cp -L "$QT_PLUGIN_DIR/platforms/libqxcb.so" "$APPDIR/usr/plugins/platforms/" 2>/dev/null || echo "    ⚠ 未找到 libqxcb.so"
fi

# 其他可能需要的 Qt 插件
mkdir -p "$APPDIR/usr/plugins/styles"
if [ -d "$QT_PLUGIN_DIR/styles" ]; then
    cp -L "$QT_PLUGIN_DIR/styles/libqgtk2style.so" "$APPDIR/usr/plugins/styles/" 2>/dev/null || true
    cp -L "$QT_PLUGIN_DIR/styles/libqwindowsvistastyle.so" "$APPDIR/usr/plugins/styles/" 2>/dev/null || true
fi

mkdir -p "$APPDIR/usr/plugins/imageformats"
if [ -d "$QT_PLUGIN_DIR/imageformats" ]; then
    for fmt in libqjpeg.so libqpng.so libqsvg.so; do
        cp -L "$QT_PLUGIN_DIR/imageformats/$fmt" "$APPDIR/usr/plugins/imageformats/" 2>/dev/null || true
    done
fi

mkdir -p "$APPDIR/usr/plugins/iconengines"
if [ -d "$QT_PLUGIN_DIR/iconengines" ]; then
    cp -L "$QT_PLUGIN_DIR/iconengines/libqsvgicon.so" "$APPDIR/usr/plugins/iconengines/" 2>/dev/null || true
fi

mkdir -p "$APPDIR/usr/plugins/xcbglintegrations"
if [ -d "$QT_PLUGIN_DIR/xcbglintegrations" ]; then
    cp -L "$QT_PLUGIN_DIR/xcbglintegrations/libqxcb-glx-integration.so" "$APPDIR/usr/plugins/xcbglintegrations/" 2>/dev/null || true
fi

mkdir -p "$APPDIR/usr/plugins/serialport"
if [ -d "$QT_PLUGIN_DIR/serialport" ]; then
    cp -L "$QT_PLUGIN_DIR/serialport/*" "$APPDIR/usr/plugins/serialport/" 2>/dev/null || true
fi

# ---- 6. 复制其他系统依赖 ----
echo "[6/7] 复制系统依赖库..."

# libusb-1.0（同星 CAN 需要）
LIBUSB=$(find /usr/lib64 /usr/lib /lib64 /lib -name "libusb-1.0.so.*" 2>/dev/null | head -1)
if [ -n "$LIBUSB" ]; then
    cp -L "$LIBUSB" "$APPDIR/usr/lib/"
    echo "    ✓ libusb-1.0"
else
    echo "    ⚠ 未找到 libusb-1.0，同星 CAN 可能无法工作"
    echo "      安装: sudo yum install libusb1-devel"
fi

# libstdc++（确保目标系统有）
LIBCRYPTO=$(find /usr/lib64 /lib64 -name "libcrypto.so.*" 2>/dev/null | head -1)
if [ -n "$LIBCRYPTO" ]; then
    cp -L "$LIBCRYPTO" "$APPDIR/usr/lib/" 2>/dev/null || true
fi

# 创建 .desktop 文件
cat > "$APPDIR/usr/share/applications/${APP_NAME}.desktop" << 'DESKTOP_EOF'
[Desktop Entry]
Name=COM and RJ45 Test
Comment=串口与 CAN 通讯测试工具
Exec=COM_and_RJ45_TEST
Icon=COM_and_RJ45_TEST
Type=Application
Categories=Development;Debugger;
Terminal=false
DESKTOP_EOF

# 复制到 AppDir 根目录（AppImage 要求）
cp "$APPDIR/usr/share/applications/${APP_NAME}.desktop" "$APPDIR/"

# 创建图标
# 用Qt的图标或一个简单的PNG
if command -v convert &>/dev/null; then
    convert -size 256x256 xc:transparent -fill blue -draw "circle 128,128 128,30" \
        "$APPDIR/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png" 2>/dev/null || true
    convert -size 48x48 xc:transparent -fill blue -draw "circle 24,24 24,6" \
        "$APPDIR/usr/share/icons/hicolor/48x48/apps/${APP_NAME}.png" 2>/dev/null || true
else
    # 创建一个1x1像素PNG作为占位
    printf '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90wS\xde\x00\x00\x00\x0cIDATx\x9cc\xf8\x0f\x00\x00\x01\x01\x00\x05\x18\xd8N\x00\x00\x00\x00IEND\xaeB`\x82' \
        > "$APPDIR/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png"
    cp "$APPDIR/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png" \
       "$APPDIR/usr/share/icons/hicolor/48x48/apps/${APP_NAME}.png"
fi

# 符号链接顶层图标
ln -sf "usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png" "$APPDIR/${APP_NAME}.png" 2>/dev/null || true

# ---- 7. 寻找并使用 linuxdeployqt 或 appimagetool ----
echo "[7/7] 准备 AppImage 打包工具..."

# 策略1: 优先使用 appimagetool（更兼容 CentOS 7）
APPIMAGETOOL="$SCRIPT_DIR/appimagetool-x86_64.AppImage"
LINUXDEPLOYQT="$SCRIPT_DIR/linuxdeployqt-x86_64.AppImage"

USE_APPIMAGETOOL=0
USE_LINUXDEPLOYQT=0

# 检查 appimagetool
if [ -f "$APPIMAGETOOL" ]; then
    USE_APPIMAGETOOL=1
    echo "    ✓ 已找到 appimagetool"
elif command -v appimagetool &>/dev/null; then
    APPIMAGETOOL="appimagetool"
    USE_APPIMAGETOOL=1
    echo "    ✓ 已找到系统 appimagetool"
fi

# 检查 linuxdeployqt
if [ -f "$LINUXDEPLOYQT" ]; then
    USE_LINUXDEPLOYQT=1
    echo "    ✓ 已找到 linuxdeployqt"
elif command -v linuxdeployqt &>/dev/null; then
    LINUXDEPLOYQT="linuxdeployqt"
    USE_LINUXDEPLOYQT=1
    echo "    ✓ 已找到系统 linuxdeployqt"
fi

# 如果都没有，下载 appimagetool（更轻量，CentOS 7 兼容性更好）
if [ $USE_APPIMAGETOOL -eq 0 ] && [ $USE_LINUXDEPLOYQT -eq 0 ]; then
    echo "    正在下载 appimagetool..."
    # appimagetool 比 linuxdeployqt 更小，更兼容旧系统
    APPIMAGE_URL="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    if command -v wget &>/dev/null; then
        wget -q --show-progress "$APPIMAGE_URL" -O "$APPIMAGETOOL" || {
            echo "    wget 失败，尝试 curl..."
            curl -L -o "$APPIMAGETOOL" "$APPIMAGE_URL"
        }
    elif command -v curl &>/dev/null; then
        curl -L -o "$APPIMAGETOOL" "$APPIMAGE_URL"
    else
        echo "❌ 错误: 需要 wget 或 curl 来下载打包工具"
        echo "   安装: sudo yum install wget curl"
        exit 1
    fi
    chmod +x "$APPIMAGETOOL"
    USE_APPIMAGETOOL=1
    echo "    ✓ 已下载 appimagetool"
fi

# ---- 创建 AppRun 启动脚本 ----
echo "    创建 AppRun 启动脚本..."
cat > "$APPDIR/AppRun" << 'APPRUN_EOF'
#!/bin/bash
# AppRun - AppImage 启动脚本
SELF_DIR="$(dirname "$(readlink -f "$0")")"

# 设置 Qt 插件路径
export QT_QPA_PLATFORM_PLUGIN_PATH="$SELF_DIR/usr/plugins"
export QT_PLUGIN_PATH="$SELF_DIR/usr/plugins"

# 设置库路径
export LD_LIBRARY_PATH="$SELF_DIR/usr/lib:$SELF_DIR/usr/bin${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# 执行程序
exec "$SELF_DIR/usr/bin/COM_and_RJ45_TEST" "$@"
APPRUN_EOF
chmod +x "$APPDIR/AppRun"

# ---- 生成 AppImage ----
echo ""
echo "========================================"
echo " 正在生成 AppImage..."
echo "========================================"

cd "$SCRIPT_DIR"

if [ $USE_APPIMAGETOOL -eq 1 ]; then
    echo "使用 appimagetool 打包..."
    # CentOS 7 FUSE 太旧，需要 --appimage-extract-and-run
    if [ -x "$APPIMAGETOOL" ] && [[ "$APPIMAGETOOL" == *.AppImage ]]; then
        # 先把 appimagetool 解压再运行（避免FUSE问题）
        APPIMAGETOOL_DIR="$SCRIPT_DIR/.appimagetool_extracted"
        rm -rf "$APPIMAGETOOL_DIR"
        "$APPIMAGETOOL" --appimage-extract >/dev/null 2>&1 || true
        if [ -d "squashfs-root" ]; then
            mv squashfs-root "$APPIMAGETOOL_DIR"
            "$APPIMAGETOOL_DIR/AppRun" "$APPDIR" "$APPIMAGE_FILE"
            rm -rf "$APPIMAGETOOL_DIR"
        else
            # 直接运行
            "$APPIMAGETOOL" "$APPDIR" "$APPIMAGE_FILE"
        fi
    else
        "$APPIMAGETOOL" "$APPDIR" "$APPIMAGE_FILE"
    fi
elif [ $USE_LINUXDEPLOYQT -eq 1 ]; then
    echo "使用 linuxdeployqt 打包..."
    export LD_LIBRARY_PATH="$APPDIR/usr/lib:$LD_LIBRARY_PATH"
    export VERSION=1.0
    # 同样处理 FUSE 兼容性
    if [ -x "$LINUXDEPLOYQT" ] && [[ "$LINUXDEPLOYQT" == *.AppImage ]]; then
        LINUXDEPLOYQT_DIR="$SCRIPT_DIR/.linuxdeployqt_extracted"
        rm -rf "$LINUXDEPLOYQT_DIR"
        "$LINUXDEPLOYQT" --appimage-extract >/dev/null 2>&1 || true
        if [ -d "squashfs-root" ]; then
            mv squashfs-root "$LINUXDEPLOYQT_DIR"
            "$LINUXDEPLOYQT_DIR/AppRun" "$APPDIR/usr/share/applications/${APP_NAME}.desktop" \
                -bundle-non-qt-libs \
                -extra-plugins=serialport \
                -appimage
            rm -rf "$LINUXDEPLOYQT_DIR"
        else
            "$LINUXDEPLOYQT" "$APPDIR/usr/share/applications/${APP_NAME}.desktop" \
                -bundle-non-qt-libs \
                -extra-plugins=serialport \
                -appimage
        fi
    else
        "$LINUXDEPLOYQT" "$APPDIR/usr/share/applications/${APP_NAME}.desktop" \
            -bundle-non-qt-libs \
            -extra-plugins=serialport \
            -appimage
    fi
fi

# 检查结果
if [ -f "$APPIMAGE_FILE" ]; then
    echo ""
    echo "========================================"
    echo " ✅ 打包成功!"
    echo "    文件: $SCRIPT_DIR/$APPIMAGE_FILE"
    echo "    大小: $(ls -lh "$APPIMAGE_FILE" | awk '{print $5}')"
    echo ""
    echo " 使用方式:"
    echo "   1. 复制到其他 CentOS 7.9 机器"
    echo "   2. chmod +x ${APPIMAGE_FILE}"
    echo "   3. ./${APPIMAGE_FILE}"
    echo ""
    echo " 如果运行时提示 FUSE 错误，添加 --appimage-extract 参数:"
    echo "   ./${APPIMAGE_FILE} --appimage-extract && ./squashfs-root/AppRun"
    echo "========================================"
else
    echo ""
    echo "❌ 打包失败！未生成 $APPIMAGE_FILE"
    echo "   请检查上面的错误信息"
    exit 1
fi
