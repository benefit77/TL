#include "libledim.h"

#include "libledim_device.h"
#include "libledim_log.h"
#include "libledim_portscan.h"

#include <stddef.h>

/*
* the first generation facade: one process wide controller
*
* this file exists for one reason only, so that a caller written against the original libledim does
* not have to change. it owns no logic of its own: every call is forwarded to the device layer, and
* the errors of the device are mirrored into a process wide slot for the times when there is no
* device to ask (a failed init, or a call made before the init).
*/

static libledim_Device* _fstatic_legacy_device = NULL;

// the timeout the caller asked for, 0 while it has never asked (the init then picks the default)
static uint32_t _fstatic_legacy_timeout_ms = 0;

// the error slot used while there is no device to carry one
static libledim_Error _fstatic_legacy_error = libledim_Error_None;
static int _fstatic_legacy_os_error = 0;

// the callback mounted through __libledim_MountunDbglogCallback(), if any
static libledim_Log_Callback _fstatic_legacy_callback = NULL;

static void __adopt_device_error (void) {
	if (_fstatic_legacy_device == NULL) {
		return;
	}
	_fstatic_legacy_error = libledim_Device_GetLastError(_fstatic_legacy_device);
	_fstatic_legacy_os_error = libledim_Device_GetLastOsError(_fstatic_legacy_device);

	return;
}

static void __set_legacy_error (libledim_Error arg_error, int arg_os_error) {
	_fstatic_legacy_error = arg_error;
	_fstatic_legacy_os_error = arg_os_error;

	return;
}

bool libledim_Init (const char* arg_com_portname) {
	if (_fstatic_legacy_device != NULL) {
		// already inited, the original answered true here as well
		return true;
	}

	libledim_Device* tmp_device = libledim_Device_Create();
	if (tmp_device == NULL) {
		__set_legacy_error(libledim_Error_OutOfMemory, 0);
		return false;
	}

	// a timeout the caller picked before the init has to survive it, and it is also the timeout the
	// probe of an explicitly named port uses (0 stands for "never picked", see libledim_SetTransactionTimeout)
	uint32_t tmp_timeout_ms = (_fstatic_legacy_timeout_ms != 0)
		? _fstatic_legacy_timeout_ms
		: libledim_MG_default_transaction_timeout;
	libledim_Device_SetTransactionTimeout(tmp_device, tmp_timeout_ms);

	if (!libledim_Device_Bind(tmp_device, arg_com_portname)) {
		__set_legacy_error(
			libledim_Device_GetLastError(tmp_device),
			libledim_Device_GetLastOsError(tmp_device)
		);
		libledim_Device_Close(tmp_device);
		return false;
	}

	_fstatic_legacy_device = tmp_device;
	__set_legacy_error(libledim_Error_None, 0);

	return true;
}

void libledim_Deinit (void) {
	if (_fstatic_legacy_device == NULL) {
		return;
	}

	libledim_Device_Close(_fstatic_legacy_device);
	_fstatic_legacy_device = NULL;

	libledim_PortScan_InvalidateCache();

	return;
}

void libledim_SetTransactionTimeout (uint32_t arg_timeout_ms) {
	_fstatic_legacy_timeout_ms = arg_timeout_ms;

	if (_fstatic_legacy_device != NULL) {
		libledim_Device_SetTransactionTimeout(_fstatic_legacy_device, arg_timeout_ms);
	}

	return;
}

const char* libledim_GetComPortname (void) {
	if (_fstatic_legacy_device == NULL) {
		return NULL;
	}

	return libledim_Device_GetComPortname(_fstatic_legacy_device);
}

bool libledim_EndisChannel (uint8_t arg_channel_order, bool arg_tof) {
	if (_fstatic_legacy_device == NULL) {
		__set_legacy_error(libledim_Error_NotInited, 0);
		return false;
	}

	bool tmp_tof = libledim_Device_EndisChannel(_fstatic_legacy_device, arg_channel_order, arg_tof);
	__adopt_device_error();

	return tmp_tof;
}

uint8_t libledim_GetChannelLuminanceLevel (uint8_t arg_channel_order) {
	if (_fstatic_legacy_device == NULL) {
		__set_legacy_error(libledim_Error_NotInited, 0);
		return 0;
	}

	uint8_t tmp_level = libledim_Device_GetChannelLuminanceLevel(_fstatic_legacy_device, arg_channel_order);
	__adopt_device_error();

	return tmp_level;
}

bool libledim_SetChannelLuminanceLevel (uint8_t arg_channel_order, uint8_t arg_channel_lumin_level) {
	if (_fstatic_legacy_device == NULL) {
		__set_legacy_error(libledim_Error_NotInited, 0);
		return false;
	}

	bool tmp_tof = libledim_Device_SetChannelLuminanceLevel(
		_fstatic_legacy_device,
		arg_channel_order,
		arg_channel_lumin_level
	);
	__adopt_device_error();

	return tmp_tof;
}

bool libledim_TriggerChannelFlash (uint8_t arg_channel_order) {
	if (_fstatic_legacy_device == NULL) {
		__set_legacy_error(libledim_Error_NotInited, 0);
		return false;
	}

	bool tmp_tof = libledim_Device_TriggerChannelFlash(_fstatic_legacy_device, arg_channel_order);
	__adopt_device_error();

	return tmp_tof;
}

bool libledim_SetChannelMode (uint8_t arg_channel_order, uint8_t arg_mode) {
	if (_fstatic_legacy_device == NULL) {
		__set_legacy_error(libledim_Error_NotInited, 0);
		return false;
	}

	bool tmp_tof = libledim_Device_SetChannelMode(_fstatic_legacy_device, arg_channel_order, arg_mode);
	__adopt_device_error();

	return tmp_tof;
}

bool libledim_SetChannelFlashDuration (uint8_t arg_channel_order, uint8_t arg_duration_ms) {
	if (_fstatic_legacy_device == NULL) {
		__set_legacy_error(libledim_Error_NotInited, 0);
		return false;
	}

	bool tmp_tof = libledim_Device_SetChannelFlashDuration(
		_fstatic_legacy_device,
		arg_channel_order,
		arg_duration_ms
	);
	__adopt_device_error();

	return tmp_tof;
}

libledim_Error libledim_GetLastError (void) {
	if (_fstatic_legacy_device != NULL) {
		return libledim_Device_GetLastError(_fstatic_legacy_device);
	}

	return _fstatic_legacy_error;
}

const char* libledim_GetLastErrorString (void) {
	return libledim_Error_ToString(libledim_GetLastError());
}

int libledim_GetLastOsError (void) {
	if (_fstatic_legacy_device != NULL) {
		return libledim_Device_GetLastOsError(_fstatic_legacy_device);
	}

	return _fstatic_legacy_os_error;
}

void libledim_SetLogCallback (libledim_Log_Callback arg_callback) {
	_fstatic_legacy_callback = arg_callback;
	libledim_Log_MountCallback(arg_callback);

	return;
}

void libledim_SetMinLogLevel (libledim_LogLevel arg_level) {
	libledim_Log_SetMinLevel(arg_level);

	return;
}

void __libledim_EndisDbglog (bool arg_tof) {
	if (!arg_tof) {
		libledim_Log_MountCallback(NULL);
		return;
	}

	// a mounted callback wins over the built-in stderr sink, exactly as the original did it
	libledim_Log_SetMinLevel(libledim_LogLevel_Debug);
	libledim_Log_MountCallback(
		(_fstatic_legacy_callback != NULL) ? _fstatic_legacy_callback : libledim_LogToStderr
	);

	return;
}

void __libledim_MountunDbglogCallback (bool arg_tof, libledim_Log_Callback arg_cb) {
	_fstatic_legacy_callback = arg_tof ? arg_cb : NULL;

	return;
}
