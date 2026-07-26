#ifndef LIBUCAN2LOADER_H
#define LIBUCAN2LOADER_H

#include <QLibrary>
#include <QString>
#include <QDebug>
#include "../libpathhelper.h"
#include "api.h"

// libucan2.so 动态加载器
// 搜索路径：AppImage目录 → app目录 → libucan2/子目录
class LibUcan2Loader
{
public:
    static bool load();
    static void unload();
    static bool isLoaded() { return s_lib && s_lib->isLoaded(); }

    // 以下为 libucan2 API 的包装
    static bool Init();
    static void Deinit();
    static bool InitChannel(uint8_t chan, bool acceptAll, uint32_t nomBaud, uint32_t dataBaud);
    static bool EndisChannel(uint8_t chan, bool enable);
    static bool SendFrame(uint8_t chan, libucan2_CANFrame *frame);
    static bool RecvFrame(uint8_t chan, libucan2_CANFrame *frame);

private:
    static QLibrary *s_lib;
    static bool s_loaded;

    // 函数指针
    typedef bool (*InitFunc)();
    typedef void (*DeinitFunc)();
    typedef bool (*InitChannelFunc)(uint8_t, bool, uint32_t, uint32_t);
    typedef bool (*EndisChannelFunc)(uint8_t, bool);
    typedef bool (*SendFrameFunc)(uint8_t, libucan2_CANFrame*);
    typedef bool (*RecvFrameFunc)(uint8_t, libucan2_CANFrame*);

    static InitFunc fp_Init;
    static DeinitFunc fp_Deinit;
    static InitChannelFunc fp_InitChannel;
    static EndisChannelFunc fp_EndisChannel;
    static SendFrameFunc fp_SendFrame;
    static RecvFrameFunc fp_RecvFrame;
};

#endif // LIBUCAN2LOADER_H
