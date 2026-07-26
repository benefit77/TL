// Windows 桩文件：提供 libucan2 函数的空实现，使程序能在 Windows 上编译链接
#include "api.h"
#include <QDebug>

extern "C" {

bool libucan2_Init(void) {
    qDebug() << "[libucan2 stub] Init (Windows: CAN not available)";
    return false;
}

void libucan2_Deinit(void) {
    qDebug() << "[libucan2 stub] Deinit";
}

bool libucan2_SetTransferTimeout(uint32_t arg_timeout_ms) {
    Q_UNUSED(arg_timeout_ms);
    return false;
}

bool libucan2_IsLastCommandSuccessful(void) {
    return false;
}

uint32_t libucan2_GetFirmwareVersion(void) {
    return 0;
}

bool libucan2_GetChannelStatus(uint8_t arg_chan_order, struct libucan2_ChannelStatus* arg_chan_status) {
    Q_UNUSED(arg_chan_order);
    Q_UNUSED(arg_chan_status);
    return false;
}

bool libucan2_InitChannel(uint8_t arg_chan_order, bool arg_is_accept_all,
                          uint32_t arg_nominal_baud_rate, uint32_t arg_data_baud_rate) {
    Q_UNUSED(arg_chan_order); Q_UNUSED(arg_is_accept_all);
    Q_UNUSED(arg_nominal_baud_rate); Q_UNUSED(arg_data_baud_rate);
    return false;
}

bool libucan2_EndisChannel(uint8_t arg_chan_order, bool arg_endis) {
    Q_UNUSED(arg_chan_order); Q_UNUSED(arg_endis);
    return false;
}

bool libucan2_SetChannelFilter(uint8_t arg_chan_order, bool arg_is_std,
                               uint32_t arg_filter_id, uint32_t arg_filter_mask) {
    Q_UNUSED(arg_chan_order); Q_UNUSED(arg_is_std);
    Q_UNUSED(arg_filter_id); Q_UNUSED(arg_filter_mask);
    return false;
}

bool libucan2_EndisChannelFilter(uint8_t arg_chan_order, bool arg_endis, bool arg_is_std) {
    Q_UNUSED(arg_chan_order); Q_UNUSED(arg_endis); Q_UNUSED(arg_is_std);
    return false;
}

bool libucan2_SendFrame(uint8_t arg_chan_order, libucan2_CANFrame* arg_can_frame) {
    Q_UNUSED(arg_chan_order); Q_UNUSED(arg_can_frame);
    return false;
}

bool libucan2_RecvFrame(uint8_t arg_chan_order, libucan2_CANFrame* arg_can_frame) {
    Q_UNUSED(arg_chan_order); Q_UNUSED(arg_can_frame);
    return false;
}

} // extern "C"
