#ifndef __libledim_h
#define __libledim_h

/*
* libledim -- control of the LED light source (illuminator) modules over a serial port
*
* this header is the whole public interface. it is self contained: nothing else needs to be
* included, and nothing else should be needed by a caller.
*
* three layers live behind it:
*   - the protocol layer: the 8 byte ascii frame that the controller firmware speaks
*   - the transport layer: a serial port, opened and driven through the host api
*   - the device layer: a port plus the frame exchange on top of it
*
* two ways to drive a controller are offered:
*   - libledim_Device_*(): one handle per controller, several controllers can be driven at once,
*     each handle carries its own error state
*   - the legacy libledim_*() functions: a single, process wide controller, the interface that the
*     first generation of libledim exposed (kept bit for bit compatible, see the README)
*
* a handle (and the legacy singleton on top of it) is not thread safe: a serial port is a single
* resource, drive each handle from one thread at a time
*/

#ifdef __cplusplus
	#include <cstdint>
	extern "C" {
#else
	#include <stdbool.h>
	#include <stdint.h>
#endif

// how the symbols are decorated depends on how the library is consumed:
//   shared library on windows  -> export while building, import while consuming
//   static library             -> nothing (define LIBLEDIM_STATIC before including this header)
//   anything else              -> a default visibility attribute
#if defined(_WIN32)
	#if defined(LIBLEDIM_STATIC)
		#define LIBLEDIM_API
	#elif defined(LIBLEDIM_BUILD_AS_DLL)
		#define LIBLEDIM_API __declspec(dllexport)
	#else
		#define LIBLEDIM_API __declspec(dllimport)
	#endif
	// the legacy api is consumed by C# through P/Invoke, so pin the calling convention instead of
	// relying on the (configurable) project default
	#define LIBLEDIM_CALL __cdecl
#else
	#define LIBLEDIM_API __attribute__((visibility("default")))
	#define LIBLEDIM_CALL
#endif

// the controller has 4 channels, numbered 1 to 4 (0 is not a channel)
#define libledim_MG_max_num_channels 4
#define libledim_MG_default_transaction_timeout 500 // ms

// vestigial, kept so that code written against the first generation of libledim still compiles
// (the buffers they used to size are an implementation detail that no longer exists)
#define libledim_MG_uart_tx_buf_size 128
#define libledim_MG_uart_rx_buf_size 128

// what went wrong, when something did
typedef enum libledim_Error {
	libledim_Error_None = 0,
	libledim_Error_NotInited,					// the controller is not open
	libledim_Error_InvalidChannel,				// a channel outside 1..libledim_MG_max_num_channels
	libledim_Error_InvalidMode,					// a mode outside 0..2
	libledim_Error_Port_NotFound,				// no serial port candidate on this host
	libledim_Error_Port_OpenFailed,				// the port could not be opened, see the os error
	libledim_Error_Port_ConfigureFailed,		// the port could not be put into 9600-8N1 raw mode
	libledim_Error_Transaction_TimedOut,		// the controller did not answer in time
	libledim_Error_Transaction_IOFailed,		// the frame could not be sent
	libledim_Error_Device_Rejected,				// the controller answered, but refused the command
	libledim_Error_Firmware_Unrecognized,		// the port answered nothing that looks like the firmware
	libledim_Error_OutOfMemory,
	libledim_Error_INVALID
} libledim_Error;

// how chatty the library is, and where its log lines go
typedef enum libledim_LogLevel {
	libledim_LogLevel_Trace = 0,
	libledim_LogLevel_Debug,
	libledim_LogLevel_Info,
	libledim_LogLevel_Warning,
	libledim_LogLevel_Error
} libledim_LogLevel;

// the library hands one formatted line to this callback, the callback decides what to do with it
typedef void (*libledim_Log_Callback) (libledim_LogLevel arg_level, const char* arg_line);

// logging is off (no callback) by default
LIBLEDIM_API void LIBLEDIM_CALL libledim_SetLogCallback (libledim_Log_Callback arg_callback);
LIBLEDIM_API void LIBLEDIM_CALL libledim_SetMinLogLevel (libledim_LogLevel arg_level);

// a ready made callback that writes every line to stderr, handy for a command line tool
// (the library itself never writes anywhere unless it is told to)
LIBLEDIM_API void LIBLEDIM_CALL libledim_LogToStderr (libledim_LogLevel arg_level, const char* arg_line);

/*
* one controller
* the handle is opaque, it is created by libledim_Device_Open() and disposed by libledim_Device_Close()
*/
typedef struct libledim_Device libledim_Device;

/*
* open a controller
* arg_com_portname:
*   - a portname, such as "/dev/ttyUSB0" or "COM2", to bind to that port
*   - NULL, to walk the ports of this host and bind to the first one whose firmware answers
* returns NULL when no firmware could be reached, see the error facilities below
* the transaction timeout of the fresh handle is libledim_MG_default_transaction_timeout
*/
LIBLEDIM_API libledim_Device* LIBLEDIM_CALL libledim_Device_Open (const char* arg_com_portname);

// close the controller and release the handle, NULL is a no-op
LIBLEDIM_API void LIBLEDIM_CALL libledim_Device_Close (libledim_Device* arg_device);

// the portname the handle is bound to, NULL for a NULL handle
LIBLEDIM_API const char* LIBLEDIM_CALL libledim_Device_GetComPortname (const libledim_Device* arg_device);

// how long a single frame exchange with the controller may take (default 500ms)
LIBLEDIM_API void LIBLEDIM_CALL libledim_Device_SetTransactionTimeout (libledim_Device* arg_device, uint32_t arg_timeout_ms);

// why the last call on this handle failed, libledim_Error_None when it succeeded
LIBLEDIM_API libledim_Error LIBLEDIM_CALL libledim_Device_GetLastError (const libledim_Device* arg_device);

// the error as an english sentence
LIBLEDIM_API const char* LIBLEDIM_CALL libledim_Device_GetLastErrorString (const libledim_Device* arg_device);

/*
* the last host level error code (errno on linux, GetLastError() on windows) behind
* libledim_Error_Port_OpenFailed/libledim_Error_Port_ConfigureFailed
* an errno of 13 (EACCES) there means the account lacks the rights on the serial port, on linux
* that is usually a missing "dialout" group membership, see the README
*/
LIBLEDIM_API int LIBLEDIM_CALL libledim_Device_GetLastOsError (const libledim_Device* arg_device);

// turn a channel on (arg_tof true) or off (false)
LIBLEDIM_API bool LIBLEDIM_CALL libledim_Device_EndisChannel (libledim_Device* arg_device, uint8_t arg_channel_order, bool arg_tof);

// read back the luminance level of a channel (0..255), 0 when the read failed (see the last error)
LIBLEDIM_API uint8_t LIBLEDIM_CALL libledim_Device_GetChannelLuminanceLevel (libledim_Device* arg_device, uint8_t arg_channel_order);

// set the luminance level of a channel (0..255)
LIBLEDIM_API bool LIBLEDIM_CALL libledim_Device_SetChannelLuminanceLevel (libledim_Device* arg_device, uint8_t arg_channel_order, uint8_t arg_channel_lumin_level);

// fire one flash on a channel (put the channel into flash mode first, see SetChannelMode)
LIBLEDIM_API bool LIBLEDIM_CALL libledim_Device_TriggerChannelFlash (libledim_Device* arg_device, uint8_t arg_channel_order);

// set the mode of a channel: 0 normal (always on), 1 external trigger, 2 flash
LIBLEDIM_API bool LIBLEDIM_CALL libledim_Device_SetChannelMode (libledim_Device* arg_device, uint8_t arg_channel_order, uint8_t arg_mode);

// how long a flash lasts, in milliseconds (0..255)
LIBLEDIM_API bool LIBLEDIM_CALL libledim_Device_SetChannelFlashDuration (libledim_Device* arg_device, uint8_t arg_channel_order, uint8_t arg_duration_ms);

/*
* the serial ports of this host
* libledim_CountComPorts() walks the host every time it is called and caches the answer, the
* portnames are then handed out by libledim_PeekComPortByIdx()
* the returned string is owned by the library and stays valid until the next count call
* neither of the two needs the controller to be open
*/
LIBLEDIM_API uint32_t LIBLEDIM_CALL libledim_CountComPorts (void);
LIBLEDIM_API const char* LIBLEDIM_CALL libledim_PeekComPortByIdx (uint32_t arg_idx);

/*
* the first generation interface: one process wide controller
* same names, same arguments, same return values and the same behaviour as the original libledim,
* so that callers (including the C# P/Invoke ones) need not change a single line
*/

// open the controller, arg_com_portname NULL means "walk the ports of this host"
LIBLEDIM_API bool LIBLEDIM_CALL libledim_Init (const char* arg_com_portname);
LIBLEDIM_API void LIBLEDIM_CALL libledim_Deinit (void);

/*
* the timeout of a single frame exchange
* a call made before libledim_Init() is honoured by the init (and by the port walk the init does),
* 0 means "not set yet", in which case the init falls back to libledim_MG_default_transaction_timeout
*/
LIBLEDIM_API void LIBLEDIM_CALL libledim_SetTransactionTimeout (uint32_t arg_timeout_ms);

// the portname the controller is bound to, NULL when libledim is not inited
LIBLEDIM_API const char* LIBLEDIM_CALL libledim_GetComPortname (void);

LIBLEDIM_API bool LIBLEDIM_CALL libledim_EndisChannel (uint8_t arg_channel_order, bool arg_tof);
LIBLEDIM_API uint8_t LIBLEDIM_CALL libledim_GetChannelLuminanceLevel (uint8_t arg_channel_order);
LIBLEDIM_API bool LIBLEDIM_CALL libledim_SetChannelLuminanceLevel (uint8_t arg_channel_order, uint8_t arg_channel_lumin_level);
LIBLEDIM_API bool LIBLEDIM_CALL libledim_TriggerChannelFlash (uint8_t arg_channel_order);
LIBLEDIM_API bool LIBLEDIM_CALL libledim_SetChannelMode (uint8_t arg_channel_order, uint8_t arg_mode);
LIBLEDIM_API bool LIBLEDIM_CALL libledim_SetChannelFlashDuration (uint8_t arg_channel_order, uint8_t arg_duration_ms);

// why the last legacy call failed, and the host level error code behind it
LIBLEDIM_API libledim_Error LIBLEDIM_CALL libledim_GetLastError (void);
LIBLEDIM_API const char* LIBLEDIM_CALL libledim_GetLastErrorString (void);
LIBLEDIM_API int LIBLEDIM_CALL libledim_GetLastOsError (void);

/*
* the debugging hooks of the first generation
* kept because they are part of the original exported interface, they are now thin wrappers around
* libledim_SetLogCallback()
*/

// true -> log everything to stderr at debug level, false -> log nothing
LIBLEDIM_API void LIBLEDIM_CALL __libledim_EndisDbglog (bool arg_tof);

// true -> route the log lines to arg_cb, false -> stop
LIBLEDIM_API void LIBLEDIM_CALL __libledim_MountunDbglogCallback (bool arg_tof, libledim_Log_Callback arg_cb);

#ifdef __cplusplus
	} // extern "C"
#endif

#endif // __libledim_h
