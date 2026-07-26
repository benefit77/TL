struct libucan2State;

#ifndef __cplusplus
typedef struct libucan2State libucan2State;
#endif

#ifndef __libucan2_internal_h
#define __libucan2_internal_h

//#include <YAC/Compat/libc/stdbool.h>
//#include <YAC/Compat/libc/stdint.h>
//#include <YAC/Program/Env.h>
//#include <YAC/Infrast/Annotation.h>
//#include <YAC/Infrast/YObject.h>
//#include <YAC/LWidget/U8Serdes.h>

#if PROG_ENV_os_windows
#include "libusb.h"
#else
#include <libucan2/libusb.h>
#endif

#include "api.h"
#include "magic.h"
#include "Comproto.h"

extern libucan2State _pglobal_lib_state;

struct U8Serdes {
    bool ic_Is_Serializer; // determined by arg_num_u8_in_buf
    uint8_t* ic_U8_Buffer_Base;
    size_t ic_Buffer_Size;
    size_t us_Num_Preserved_Head_U8; // horribly named, u8 at the beginning of buffer that are free from ser and des
    size_t us_Num_Preserved_Tail_U8; // num preserved tail u8 is not accounted for num_u8_in_buffer
    size_t prop_Data_Len; // this field is much like a cursor, for serializer, this is the number of bytes serialized, for deserializer, is the number of bytes yet to deserialize
};

struct libucan2State {
    uint8_t __yobj_state;

    uint32_t us_Transfer_Timeout_Ms;
    libucan2_Dbglog_Callback mnt_Dbglog_Callback;
    uint8_t ic_Ping_Random_Byteseq[libucan2_MG_ping_random_byteseq_len];

    bool egg_Is_libusb_Inited;
    libusb_device_handle* egg_libusb_device_handle;

    uint8_t dmy_Hid_In_Report_Buf[libucan2_MG_usb_hid_in_report_buf_size];
    uint8_t dmy_Hid_Out_Report_Buf[libucan2_MG_usb_hid_out_report_buf_size];
    U8Serdes bi_Hid_In_Report_U8Serdes;
    U8Serdes bi_Hid_Out_Report_U8Serdes;

    Comproto_Opcode prop_Tx_Opcode;
    uint8_t prop_Tx_Frame_Sernum;

    enum libusb_error cov_Last_libusb_Error_Code; // enum libusb_error
    bool prop_Is_Last_Command_Successful;

    libucan2_ChannelStatus dmy_Cached_Channel_Status;
    libucan2_CANFrame dmy_Cached_Rx_CAN_Frame;
    char dmy_CAN_Frame_Data_Hex_CStr_Buf[129]; // 64-byte can frame data, 2 char for 1 byte, plus the trailing '\0'
};

extern void internal_reset_library_state (bool arg_is_por);
extern bool internal_is_can_frame_valid (libucan2_CANFrame* arg_can_frame);
extern void internal_log_can_frame (libucan2_CANFrame* arg_can_frame);

#endif // __libucan2_internal_h
