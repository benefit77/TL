#if defined(_WIN32) && !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0600
#endif

#include "LibUcan2Loader.h"

#include <QDir>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <string>

namespace {
std::wstring g_previousDllDirectory;
bool g_hasPreviousDllDirectory = false;
bool g_dllDirectoryChanged = false;

// QLibrary uses LoadLibrary internally. Its dependency search path does not
// include the directory containing the DLL, so temporarily add it here.
void setDllDirectoryForLibrary(const QString &libPath)
{
    if (g_dllDirectoryChanged)
        return;

    const DWORD required = GetDllDirectoryW(0, nullptr);
    if (required > 0) {
        std::wstring buffer(required + 1, L'\0');
        const DWORD length = GetDllDirectoryW(required + 1, &buffer[0]);
        if (length > 0 && length <= required) {
            buffer.resize(length);
            g_previousDllDirectory.swap(buffer);
            g_hasPreviousDllDirectory = true;
        }
    }

    const QString nativeDir =
        QDir::toNativeSeparators(QFileInfo(libPath).absolutePath());
    g_dllDirectoryChanged =
        SetDllDirectoryW(reinterpret_cast<const wchar_t *>(nativeDir.utf16())) != 0;
}

void restoreDllDirectory()
{
    if (!g_dllDirectoryChanged)
        return;

    SetDllDirectoryW(g_hasPreviousDllDirectory ? g_previousDllDirectory.c_str()
                                               : nullptr);
    g_previousDllDirectory.clear();
    g_hasPreviousDllDirectory = false;
    g_dllDirectoryChanged = false;
}
} // namespace
#endif

QLibrary *LibUcan2Loader::s_lib = nullptr;
bool LibUcan2Loader::s_loaded = false;

LibUcan2Loader::InitFunc        LibUcan2Loader::fp_Init = nullptr;
LibUcan2Loader::DeinitFunc      LibUcan2Loader::fp_Deinit = nullptr;
LibUcan2Loader::InitChannelFunc LibUcan2Loader::fp_InitChannel = nullptr;
LibUcan2Loader::EndisChannelFunc LibUcan2Loader::fp_EndisChannel = nullptr;
LibUcan2Loader::SendFrameFunc   LibUcan2Loader::fp_SendFrame = nullptr;
LibUcan2Loader::RecvFrameFunc   LibUcan2Loader::fp_RecvFrame = nullptr;

bool LibUcan2Loader::load()
{
    if (s_loaded) return true;

    // 库文件名：Windows 用 libucan2.dll，其它平台用 libucan2.so
#ifdef Q_OS_WIN
    const QString libName = QStringLiteral("libucan2.dll");
#else
    const QString libName = QStringLiteral("libucan2.so");
#endif

    // 搜索路径：AppImage 目录 → app 目录 → libucan2/ 子目录
    QString libPath = findLibFile(libName, "libucan2");
    if (libPath.isEmpty())
        libPath = findLibFile(libName);

    if (libPath.isEmpty()) {
        qDebug() << "[LibUcan2] 未找到" << libName;
        return false;
    }

#ifdef Q_OS_WIN
    setDllDirectoryForLibrary(libPath);
#endif

    s_lib = new QLibrary(libPath);
    if (!s_lib->load()) {
        qDebug() << "[LibUcan2] 加载失败:" << s_lib->errorString();
        delete s_lib;
        s_lib = nullptr;
#ifdef Q_OS_WIN
        restoreDllDirectory();
#endif
        return false;
    }

    fp_Init        = (InitFunc)s_lib->resolve("libucan2_Init");
    fp_Deinit      = (DeinitFunc)s_lib->resolve("libucan2_Deinit");
    fp_InitChannel = (InitChannelFunc)s_lib->resolve("libucan2_InitChannel");
    fp_EndisChannel = (EndisChannelFunc)s_lib->resolve("libucan2_EndisChannel");
    fp_SendFrame   = (SendFrameFunc)s_lib->resolve("libucan2_SendFrame");
    fp_RecvFrame   = (RecvFrameFunc)s_lib->resolve("libucan2_RecvFrame");

    if (!fp_Init || !fp_Deinit || !fp_SendFrame || !fp_RecvFrame) {
        qDebug() << "[LibUcan2] 函数解析失败";
        unload();
        return false;
    }

    s_loaded = true;
    qDebug() << "[LibUcan2] 动态加载成功:" << libPath;
    return true;
}

void LibUcan2Loader::unload()
{
    if (s_lib) {
        if (s_lib->isLoaded()) s_lib->unload();
        delete s_lib;
        s_lib = nullptr;
    }
    s_loaded = false;
    fp_Init = nullptr;
    fp_Deinit = nullptr;
    fp_InitChannel = nullptr;
    fp_EndisChannel = nullptr;
    fp_SendFrame = nullptr;
    fp_RecvFrame = nullptr;

#ifdef Q_OS_WIN
    restoreDllDirectory();
#endif
}

bool LibUcan2Loader::Init() { return fp_Init ? fp_Init() : false; }
void LibUcan2Loader::Deinit() { if (fp_Deinit) fp_Deinit(); }
bool LibUcan2Loader::InitChannel(uint8_t chan, bool acceptAll, uint32_t nomBaud, uint32_t dataBaud) {
    return fp_InitChannel ? fp_InitChannel(chan, acceptAll, nomBaud, dataBaud) : false;
}
bool LibUcan2Loader::EndisChannel(uint8_t chan, bool enable) {
    return fp_EndisChannel ? fp_EndisChannel(chan, enable) : false;
}
bool LibUcan2Loader::SendFrame(uint8_t chan, libucan2_CANFrame *frame) {
    return fp_SendFrame ? fp_SendFrame(chan, frame) : false;
}
bool LibUcan2Loader::RecvFrame(uint8_t chan, libucan2_CANFrame *frame) {
    return fp_RecvFrame ? fp_RecvFrame(chan, frame) : false;
}
