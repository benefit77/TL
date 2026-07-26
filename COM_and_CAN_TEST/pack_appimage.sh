#!/bin/bash
# ============================================================
# 一键打包 COM_and_RJ45_TEST 为 AppImage
# 在 Ubuntu 上运行:  chmod +x pack_appimage.sh && ./pack_appimage.sh
# ============================================================
set -e

APP_NAME="COM_and_RJ45_TEST"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APPIMAGE_FILE="${APP_NAME}-x86_64.AppImage"

echo "========================================"
echo " 打包 $APP_NAME 为 AppImage"
echo "========================================"

# ---- 1. 检查依赖 ----
echo "[1/6] 检查编译环境..."
command -v qmake >/dev/null 2>&1 || { echo "错误: 未安装 qmake，请先: sudo apt install qtbase5-dev qt5-qmake libqt5serialport5-dev"; exit 1; }
command -v make >/dev/null 2>&1 || { echo "错误: 未安装 make"; exit 1; }

# ---- 2. 清理 Windows 残留 ----
echo "[2/6] 清理 Windows 残留文件..."
rm -f "$SCRIPT_DIR"/*.pro.user*
rm -f "$SCRIPT_DIR"/release/*.o "$SCRIPT_DIR"/release/*.exe "$SCRIPT_DIR"/release/*.dll 2>/dev/null
rm -rf "$SCRIPT_DIR"/build 2>/dev/null

# ---- 3. 编译 ----
echo "[3/6] 编译项目..."
cd "$SCRIPT_DIR"
qmake "$APP_NAME.pro"
make -j$(nproc)
echo "      编译完成: $APP_NAME"

# ---- 4. 准备 AppDir ----
echo "[4/6] 准备 AppDir 目录..."
APPDIR="$SCRIPT_DIR/AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

# 复制可执行文件
cp "$APP_NAME" "$APPDIR/usr/bin/"

# 复制 CAN 动态库（AppImage 外部查找的备用，也打包进 AppDir）
mkdir -p "$APPDIR/usr/bin/cx"
mkdir -p "$APPDIR/usr/bin/zy"
mkdir -p "$APPDIR/usr/bin/libucan2"
if [ -f "cx/libcontrolcan.so" ]; then cp "cx/libcontrolcan.so" "$APPDIR/usr/bin/cx/"; fi
if [ -f "zy/libusbcan.so" ]; then cp "zy/libusbcan.so" "$APPDIR/usr/bin/zy/"; fi
if [ -f "libucan2/libucan2.so" ]; then cp "libucan2/libucan2.so" "$APPDIR/usr/bin/libucan2/"; fi
if [ -f "libucan2/libucan2_dev2.so" ]; then cp "libucan2/libucan2_dev2.so" "$APPDIR/usr/bin/libucan2/"; fi

# 创建 .desktop 文件（AppImage 必需）
cat > "$APPDIR/usr/share/applications/${APP_NAME}.desktop" << EOF
[Desktop Entry]
Name=COM and RJ45 Test
Comment=串口与 CAN 通讯测试工具
Exec=${APP_NAME}
Icon=${APP_NAME}
Type=Application
Categories=Development;Debugger;
Terminal=false
EOF

# 创建图标（使用默认图标）
convert -size 256x256 xc:transparent "$APPDIR/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png" 2>/dev/null || true

# 复制 .desktop 到顶层（linuxdeployqt 要求）
cp "$APPDIR/usr/share/applications/${APP_NAME}.desktop" "$APPDIR/"
ln -sf "usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png" "$APPDIR/${APP_NAME}.png" 2>/dev/null || true

# ---- 5. 下载 linuxdeployqt（如果不存在）----
echo "[5/6] 检查 linuxdeployqt..."
LINUXDEPLOYQT="$SCRIPT_DIR/linuxdeployqt-x86_64.AppImage"
if [ ! -f "$LINUXDEPLOYQT" ]; then
    echo "      正在下载 linuxdeployqt..."
    wget -q --show-progress \
        "https://github.com/linuxdeploy/linuxdeployqt/releases/download/continuous/linuxdeployqt-x86_64.AppImage" \
        -O "$LINUXDEPLOYQT"
    chmod +x "$LINUXDEPLOYQT"
fi

# ---- 6. 打包 AppImage ----
echo "[6/6] 正在生成 AppImage..."
cd "$SCRIPT_DIR"

# 运行 linuxdeployqt
export PATH="$APPDIR/usr/bin:$PATH"
$LINUXDEPLOYQT "$APPDIR/usr/share/applications/${APP_NAME}.desktop" \
    -bundle-non-qt-libs \
    -extra-plugins=serialport \
    -appimage

# 检查结果
if [ -f "$APPIMAGE_FILE" ]; then
    echo ""
    echo "========================================"
    echo " ✅ 打包成功!"
    echo "    文件: $SCRIPT_DIR/$APPIMAGE_FILE"
    echo "    大小: $(du -h "$APPIMAGE_FILE" | cut -f1)"
    echo ""
    echo " 运行:  ./${APPIMAGE_FILE}"
    echo ""
    echo " CAN 库搜索路径（优先级）:"
    echo "  1. AppImage 同目录下的 cx/ zy/ libucan2/"
    echo "  2. AppImage 内部（已打包）"
    echo "========================================"
else
    echo "❌ 打包失败，请检查错误信息"
    exit 1
fi
