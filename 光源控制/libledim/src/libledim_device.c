#include "libledim_device.h"

#include "libledim_log.h"
#include "libledim_proto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void __set_error (libledim_Device* arg_device, libledim_Error arg_error, int arg_os_error) {
	if (arg_device == NULL) {
		return;
	}
	arg_device->prop_last_error = arg_error;
	arg_device->prop_last_os_error = arg_os_error;

	return;
}

const char* libledim_Error_ToString (libledim_Error arg_error) {
	switch (arg_error) {
	case libledim_Error_None:
		return "no error";
	case libledim_Error_NotInited:
		return "the controller is not open";
	case libledim_Error_InvalidChannel:
		return "no such channel, the channels are numbered 1 to 4";
	case libledim_Error_InvalidMode:
		return "no such mode, the modes are 0 (normal), 1 (external trigger) and 2 (flash)";
	case libledim_Error_Port_NotFound:
		return "no serial port was found on this host";
	case libledim_Error_Port_OpenFailed:
		return "the serial port could not be opened";
	case libledim_Error_Port_ConfigureFailed:
		return "the serial port could not be set to 9600-8N1";
	case libledim_Error_Transaction_TimedOut:
		return "the controller did not answer in time";
	case libledim_Error_Transaction_IOFailed:
		return "the request frame could not be sent";
	case libledim_Error_Device_Rejected:
		return "the controller refused the command";
	case libledim_Error_Firmware_Unrecognized:
		return "no controller answered on this port";
	case libledim_Error_OutOfMemory:
		return "out of memory";
	default:
		return "unknown error";
	}
}

libledim_Device* libledim_Device_Create (void) {
	libledim_Device* tmp_device = (libledim_Device*)malloc(sizeof(libledim_Device));
	if (tmp_device == NULL) {
		return NULL;
	}

	memset(tmp_device, 0, sizeof(libledim_Device));
	tmp_device->us_transaction_timeout_ms = libledim_MG_default_transaction_timeout;
	tmp_device->prop_last_error = libledim_Error_None;

	return tmp_device;
}

bool libledim_Device_IsOpen (const libledim_Device* arg_device) {
	return (arg_device != NULL && arg_device->comp_transport_ctx != NULL);
}

void libledim_Device_Close (libledim_Device* arg_device) {
	if (arg_device == NULL) {
		return;
	}

	if (arg_device->comp_transport_ctx != NULL) {
		arg_device->comp_transport->close(arg_device->comp_transport_ctx);
		arg_device->comp_transport_ctx = NULL;
	}

	free(arg_device);

	return;
}

/*
* one frame exchange: send the request, read the reply, hand back what it carried
*
* this is the only place that talks to the wire, and it mirrors the original implementation step
* for step: drop whatever is buffered, send all 8 bytes, read the whole reply (8 bytes for the
* luminance read back, 1 byte otherwise), then read the answer out of it
*/
static bool __transact (
	libledim_Device* arg_device,
	libledim_Opcode arg_opcode,
	uint8_t arg_channel_order,
	uint8_t arg_op_arg,
	uint8_t* arg_ret_retval
) {
	if (arg_device == NULL || !libledim_Device_IsOpen(arg_device)) {
		__set_error(arg_device, libledim_Error_NotInited, 0);
		return false;
	}
	if (arg_channel_order == 0 || arg_channel_order > libledim_MG_max_num_channels) {
		libledim_Log_Printf(
			libledim_LogLevel_Error,
			"invalid channel order %u, the channels are numbered 1 to %d\n",
			arg_channel_order,
			libledim_MG_max_num_channels
		);
		__set_error(arg_device, libledim_Error_InvalidChannel, 0);
		return false;
	}

	uint8_t tmp_tx_frame[libledim_Proto_Tx_Frame_Len] = { 0 };
	libledim_Proto_EncodeFrame(arg_opcode, arg_channel_order, arg_op_arg, tmp_tx_frame);

	// the frame itself, before it goes out: the one line that makes a serial protocol readable in
	// a log, and what the log pane of the graphical tool shows as it runs
	libledim_Log_Printf(
		libledim_LogLevel_Debug,
		"request: %.*s\n",
		(int)libledim_Proto_Tx_Frame_Len,
		(const char*)tmp_tx_frame
	);

	// the controller has nothing to say about frames it received before the command
	arg_device->comp_transport->flush(arg_device->comp_transport_ctx);

	uint32_t tmp_num_written = arg_device->comp_transport->write(
		arg_device->comp_transport_ctx,
		tmp_tx_frame,
		libledim_Proto_Tx_Frame_Len,
		arg_device->us_transaction_timeout_ms
	);
	if (tmp_num_written < libledim_Proto_Tx_Frame_Len) {
		libledim_Log_Printf(
			libledim_LogLevel_Error,
			"the request frame could not be sent (%u of %u bytes, os error %d)\n",
			tmp_num_written,
			libledim_Proto_Tx_Frame_Len,
			arg_device->prop_last_os_error
		);
		__set_error(arg_device, libledim_Error_Transaction_IOFailed, 0);
		return false;
	}

	uint32_t tmp_reply_len = libledim_Proto_ReplyLenOf(arg_opcode);
	uint8_t tmp_rx_frame[libledim_Proto_Rx_Frame_Len] = { 0 };
	uint32_t tmp_num_read = arg_device->comp_transport->read(
		arg_device->comp_transport_ctx,
		tmp_rx_frame,
		tmp_reply_len,
		arg_device->us_transaction_timeout_ms
	);
	if (tmp_num_read < tmp_reply_len) {
		libledim_Log_Printf(
			libledim_LogLevel_Error,
			"the controller did not answer in time (%u of %u bytes)\n",
			tmp_num_read,
			tmp_reply_len
		);
		__set_error(arg_device, libledim_Error_Transaction_TimedOut, 0);
		return false;
	}

	__set_error(arg_device, libledim_Error_None, 0);

	if (arg_opcode == libledim_Opcode_GetChannelLuminanceLevel) {
		// the luminance read back is answered with the value itself, there is no retcode to decode
		if (arg_ret_retval != NULL) {
			*arg_ret_retval = libledim_Proto_DecodeLuminanceReply(tmp_rx_frame);
		}
		return true;
	}

	if (!libledim_Proto_DecodeRetcode(tmp_rx_frame[0])) {
		libledim_Log_Printf(
			libledim_LogLevel_Error,
			"the controller refused the command (opcode '%c', channel %u)\n",
			(char)arg_opcode,
			arg_channel_order
		);
		__set_error(arg_device, libledim_Error_Device_Rejected, 0);
		return false;
	}

	return true;
}

/*
* open arg_portname and shake hands with whatever is on the other side
*
* the handshake is the original one: ask for the luminance of channel 1. a controller answers that
* with 8 bytes, and something that is not a controller (a modem, a console, a bare uart) either
* says nothing at all or says something too short, which is exactly what tells the two apart
*/
static bool __probe_port (libledim_Device* arg_device, const char* arg_portname, uint32_t arg_timeout_ms) {
	if (arg_device->comp_transport_ctx != NULL) {
		arg_device->comp_transport->close(arg_device->comp_transport_ctx);
		arg_device->comp_transport_ctx = NULL;
	}

	libledim_Log_Printf(libledim_LogLevel_Info, "opening %s\n", arg_portname);

	void* tmp_ctx = NULL;
	int tmp_os_error = libledim_Transport_Host.open(&tmp_ctx, arg_portname);
	if (tmp_os_error != 0) {
		libledim_Log_Printf(
			libledim_LogLevel_Error,
			"opening %s failed (os error %d)\n",
			arg_portname,
			tmp_os_error
		);
		__set_error(arg_device, libledim_Error_Port_OpenFailed, tmp_os_error);
		return false;
	}

	arg_device->comp_transport = &libledim_Transport_Host;
	arg_device->comp_transport_ctx = tmp_ctx;
	snprintf(
		arg_device->ic_com_portname,
		sizeof(arg_device->ic_com_portname),
		"%s",
		arg_portname
	);

	uint32_t tmp_saved_timeout_ms = arg_device->us_transaction_timeout_ms;
	arg_device->us_transaction_timeout_ms = arg_timeout_ms;

	libledim_Log_Printf(libledim_LogLevel_Info, "checking for the controller firmware\n");
	uint8_t tmp_dummy_level = 0;
	bool tmp_is_controller = __transact(
		arg_device,
		libledim_Opcode_GetChannelLuminanceLevel,
		1,
		0,
		&tmp_dummy_level
	);

	arg_device->us_transaction_timeout_ms = tmp_saved_timeout_ms;

	if (!tmp_is_controller) {
		libledim_Log_Printf(libledim_LogLevel_Warning, "%s does not hold a controller\n", arg_portname);
		arg_device->comp_transport->close(arg_device->comp_transport_ctx);
		arg_device->comp_transport_ctx = NULL;
		__set_error(arg_device, libledim_Error_Firmware_Unrecognized, 0);
		return false;
	}

	return true;
}

bool libledim_Device_Bind (libledim_Device* arg_device, const char* arg_com_portname) {
	if (arg_device == NULL) {
		return false;
	}
	if (arg_com_portname != NULL) {
		return __probe_port(arg_device, arg_com_portname, arg_device->us_transaction_timeout_ms);
	}

	libledim_Log_Printf(
		libledim_LogLevel_Info,
		"no portname given, walking the serial ports of this host\n"
	);

	libledim_PortnameList tmp_portnames = { NULL, 0 };
	libledim_PortScan_CollectHostPortnames(&tmp_portnames);

	if (tmp_portnames.count == 0) {
		libledim_Log_Printf(libledim_LogLevel_Error, "this host exposes no serial port at all\n");
		__set_error(arg_device, libledim_Error_Port_NotFound, 0);
		return false;
	}

	bool tmp_found = false;
	for (uint32_t tmp_i = 0; tmp_i < tmp_portnames.count; tmp_i++) {
		if (__probe_port(arg_device, tmp_portnames.names[tmp_i], libledim_Device_MG_probe_timeout_ms)) {
			tmp_found = true;
			break;
		}
	}
	libledim_PortScan_FreeList(&tmp_portnames);

	if (!tmp_found) {
		// the walk is over, so no port held a controller; keep the open failure when that is what
		// went wrong, since "the account lacks the rights on the port" is far more actionable than
		// "nothing answered", and it is the usual cause on linux (see the README)
		if (arg_device->prop_last_error != libledim_Error_Port_OpenFailed) {
			__set_error(arg_device, libledim_Error_Firmware_Unrecognized, 0);
		}
		return false;
	}

	return true;
}

libledim_Device* libledim_Device_Open (const char* arg_com_portname) {
	libledim_Device* tmp_device = libledim_Device_Create();
	if (tmp_device == NULL) {
		return NULL;
	}

	if (!libledim_Device_Bind(tmp_device, arg_com_portname)) {
		libledim_Device_Close(tmp_device);
		return NULL;
	}

	return tmp_device;
}

const char* libledim_Device_GetComPortname (const libledim_Device* arg_device) {
	if (!libledim_Device_IsOpen(arg_device)) {
		return NULL;
	}

	return arg_device->ic_com_portname;
}

void libledim_Device_SetTransactionTimeout (libledim_Device* arg_device, uint32_t arg_timeout_ms) {
	if (arg_device == NULL) {
		return;
	}
	arg_device->us_transaction_timeout_ms = arg_timeout_ms;

	return;
}

libledim_Error libledim_Device_GetLastError (const libledim_Device* arg_device) {
	if (arg_device == NULL) {
		return libledim_Error_NotInited;
	}

	return arg_device->prop_last_error;
}

const char* libledim_Device_GetLastErrorString (const libledim_Device* arg_device) {
	if (arg_device == NULL) {
		return "no controller";
	}

	return libledim_Error_ToString(arg_device->prop_last_error);
}

int libledim_Device_GetLastOsError (const libledim_Device* arg_device) {
	if (arg_device == NULL) {
		return 0;
	}

	return arg_device->prop_last_os_error;
}

bool libledim_Device_EndisChannel (libledim_Device* arg_device, uint8_t arg_channel_order, bool arg_tof) {
	return __transact(
		arg_device,
		arg_tof ? libledim_Opcode_TurnOnChannel : libledim_Opcode_TurnOffChannel,
		arg_channel_order,
		0,
		NULL
	);
}

uint8_t libledim_Device_GetChannelLuminanceLevel (libledim_Device* arg_device, uint8_t arg_channel_order) {
	uint8_t tmp_level = 0;

	__transact(
		arg_device,
		libledim_Opcode_GetChannelLuminanceLevel,
		arg_channel_order,
		0,
		&tmp_level
	);

	// note that 0 is what a failed read answers as well, libledim_Device_GetLastError() tells the
	// two apart (this is the behaviour of the first generation, kept for the callers' sake)
	return tmp_level;
}

bool libledim_Device_SetChannelLuminanceLevel (libledim_Device* arg_device, uint8_t arg_channel_order, uint8_t arg_channel_lumin_level) {
	return __transact(
		arg_device,
		libledim_Opcode_SetChannelLuminanceLevel,
		arg_channel_order,
		arg_channel_lumin_level,
		NULL
	);
}

bool libledim_Device_TriggerChannelFlash (libledim_Device* arg_device, uint8_t arg_channel_order) {
	return __transact(
		arg_device,
		libledim_Opcode_TriggerChannelFlash,
		arg_channel_order,
		0,
		NULL
	);
}

bool libledim_Device_SetChannelMode (libledim_Device* arg_device, uint8_t arg_channel_order, uint8_t arg_mode) {
	if (arg_mode > 2) {
		libledim_Log_Printf(
			libledim_LogLevel_Error,
			"invalid mode %u (0 normal, 1 external trigger, 2 flash)\n",
			arg_mode
		);
		__set_error(arg_device, libledim_Error_InvalidMode, 0);
		return false;
	}

	return __transact(
		arg_device,
		libledim_Opcode_SetChannelMode,
		arg_channel_order,
		arg_mode,
		NULL
	);
}

bool libledim_Device_SetChannelFlashDuration (libledim_Device* arg_device, uint8_t arg_channel_order, uint8_t arg_duration_ms) {
	return __transact(
		arg_device,
		libledim_Opcode_SetChannelFlashDuration,
		arg_channel_order,
		arg_duration_ms,
		NULL
	);
}
