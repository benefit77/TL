#include "LibUcan2Loader.h"

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

    // 搜索路径：AppImage 目录 → app 目录 → libucan2/ 子目录
    QString libPath = findLibFile("libucan2.so", "libucan2");
    if (libPath.isEmpty())
        libPath = findLibFile("libucan2.so");

    if (libPath.isEmpty()) {
        qDebug() << "[LibUcan2] 未找到 libucan2.so";
        return false;
    }

    s_lib = new QLibrary(libPath);
    if (!s_lib->load()) {
        qDebug() << "[LibUcan2] 加载失败:" << s_lib->errorString();
        delete s_lib;
        s_lib = nullptr;
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
