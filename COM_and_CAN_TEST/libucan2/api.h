#ifndef __libucan2_api_h
#define __libucan2_api_h

//#define LIB_BC_debug			// uncomment this line to enable sipilab
//#define LIB_BC_build_as_dll	// this line is only a hint, do not uncomment to define this marco here, which is presumably defined only at the build system level, not at some ".h"

#ifdef __cplusplus
    #include <cstdint>
    #define libucan2_EXTERN_C extern "C"
#else
    #include <stdbool.h>
    #include <stdint.h>
    #define libucan2_EXTERN_C extern
#endif

#ifdef _MSC_VER
    #ifdef LIB_BC_build_as_dll
        #define libucan2_EXPORT_API __declspec(dllexport)
    #else
        #define libucan2_EXPORT_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__)
    #ifdef LIB_BC_build_as_dll
        #define libucan2_EXPORT_API __attribute__((visibility("default")))
    #else
        #define libucan2_EXPORT_API
    #endif
#else
    #error not supported compiler, implement libucan2_EXPORT_API by yourself
#endif

struct libucan2_ChannelStatus {
    uint8_t ChannelOrder;
    bool IsEnabled;
    bool IsAcceptingAllFrames;
    bool IsStdFilterEnabled;
    bool IsExtFilterEnabled;
    uint8_t NumRxFrames;
    uint32_t NominalBaudrate;
    uint32_t DataBaudrate;
    uint32_t StdFilterId;
    uint32_t StdFilterMask;
    uint32_t ExtFilterId;
    uint32_t ExtFilterMask;
};
struct libucan2_CANFrame {
    uint32_t MsgId;
    bool IsStdId;
    bool IsClassicFrame;
    bool IsDataFrame;
    bool UseBRS;
    uint8_t DataLength;
    unsigned char Data[64];
};

#ifndef __cplusplus
    typedef struct libucan2_ChannelStatus libucan2_ChannelStatus;
    typedef struct libucan2_CANFrame libucan2_CANFrame;
#endif

libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_Init ();
libucan2_EXTERN_C libucan2_EXPORT_API void libucan2_Deinit ();

libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_SetTransferTimeout (uint32_t arg_timeout_ms);
libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_IsLastCommandSuccessful ();

libucan2_EXTERN_C libucan2_EXPORT_API uint32_t libucan2_GetFirmwareVersion ();

libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_GetChannelStatus (uint8_t arg_chan_order, struct libucan2_ChannelStatus* arg_chan_status);
//libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_IsChannelEnabled (uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_IsChannelAcceptingAllFrame (uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_IsChannelStdFilterEnabled (uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_IsChannelExtFilterEnabled (uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API u8 libucan2_GetChannelNumRxFrames (uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API u32 libucan2_GetChannelNominalBaudrate (uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API u32 libucan2_GetChannelDataBaudrate (uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API u32 libucan2_GetChannelStdFilterId (uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API u32 libucan2_GetChannelStdFilterMask (uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API u32 libucan2_GetChannelExtFilterId(uint8_t arg_chan_order);
//libucan2_EXTERN_C libucan2_EXPORT_API u32 libucan2_GetChannelExtFilterMask(uint8_t arg_chan_order);

libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_InitChannel (
    uint8_t arg_chan_order,
    bool arg_is_accept_all,
    uint32_t arg_nominal_baud_rate,
    uint32_t arg_data_baud_rate
);
libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_EndisChannel (uint8_t arg_chan_order, bool arg_endis);

libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_SetChannelFilter (uint8_t arg_chan_order, bool arg_is_std, uint32_t arg_filter_id, uint32_t arg_filter_mask);
libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_EndisChannelFilter (uint8_t arg_chan_order, bool arg_endis, bool arg_is_std);

libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_SendFrame (uint8_t arg_chan_order, libucan2_CANFrame* arg_can_frame);
libucan2_EXTERN_C libucan2_EXPORT_API bool libucan2_RecvFrame (uint8_t arg_chan_order, libucan2_CANFrame* arg_can_frame);

#ifdef __cplusplus
    using libucan2_Dbglog_Callback = void (const char* arg_logline_cstr);
#else
    typedef void (*libucan2_Dbglog_Callback)(const char* arg_logline_cstr);
#endif
libucan2_EXTERN_C libucan2_EXPORT_API void __libucan2_MountunDbglogCallback (bool arg_tof, libucan2_Dbglog_Callback arg_cb);
libucan2_EXTERN_C libucan2_EXPORT_API void __libucan2_EndisDbglog (bool arg_tof);

#endif // __libucan2_api_h
