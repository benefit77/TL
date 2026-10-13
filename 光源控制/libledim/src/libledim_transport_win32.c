#ifdef _WIN32

#include "libledim_transport.h"

/*
* the win32 comm transport
*
* note: this file is not compiled, and therefore not exercised, by the linux build this rewrite was
* developed and tested on. it mirrors the win32 backend of the original libledim (same DCB fields,
* same COMMTIMEOUTS, same PURGE flags), which is the part that is known to work against the
* firmware, so keep the two in step when touching either.
*/

#include <stdlib.h>
#include <string.h>

#include <windows.h>

typedef struct libledim_Win32TransportCtx {
	HANDLE com_handle;
	DWORD last_set_timeout_ms;
} libledim_Win32TransportCtx;

static int __set_com_timeout (libledim_Win32TransportCtx* arg_ctx, uint32_t arg_timeout_ms) {
	if (arg_ctx->last_set_timeout_ms == arg_timeout_ms && arg_timeout_ms > 0) {
		// SetCommTimeouts() is a relatively expensive ioctl, and it is called on every transaction
		return 0;
	}

	COMMTIMEOUTS tmp_commtimeouts = {
		.ReadIntervalTimeout = 0,
		.ReadTotalTimeoutMultiplier = 0,
		.ReadTotalTimeoutConstant = arg_timeout_ms,
		.WriteTotalTimeoutMultiplier = 0,
		.WriteTotalTimeoutConstant = arg_timeout_ms
	};

	if (!SetCommTimeouts(arg_ctx->com_handle, &tmp_commtimeouts)) {
		return (int)GetLastError();
	}

	arg_ctx->last_set_timeout_ms = arg_timeout_ms;

	return 0;
}

static int __open (void** arg_ret_ctx, const char* arg_portname) {
	libledim_Win32TransportCtx* tmp_ctx = NULL;

	// a "full portname" is prefixed with 4 characters, "\\.\", to reach past the com1..com9 aliases
	char tmp_full_portname[512] = "\\\\.\\";
	size_t tmp_portname_len = strlen(arg_portname);
	if (tmp_portname_len > sizeof(tmp_full_portname) - 5) {
		return ERROR_BAD_PATHNAME;
	}
	memcpy(tmp_full_portname + 4, arg_portname, tmp_portname_len + 1);

	HANDLE tmp_com_handle = CreateFileA(
		tmp_full_portname,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0, // synchronous, non overlapped io
		NULL
	);
	if (tmp_com_handle == INVALID_HANDLE_VALUE) {
		return (int)GetLastError();
	}

	DCB tmp_dcb = { 0 };
	tmp_dcb.DCBlength = sizeof(DCB);

	// 9600-8N1, no flow control
	tmp_dcb.BaudRate = CBR_9600;
	tmp_dcb.ByteSize = 8;
	tmp_dcb.Parity = NOPARITY;
	tmp_dcb.StopBits = ONESTOPBIT;
	tmp_dcb.fRtsControl = RTS_CONTROL_DISABLE;
	// the modem control lines are left alone, exactly as the original backend did
	tmp_dcb.fBinary = TRUE;

	if (!SetCommState(tmp_com_handle, &tmp_dcb)) {
		int tmp_errcode = (int)GetLastError();
		CloseHandle(tmp_com_handle);
		return tmp_errcode;
	}

	tmp_ctx = (libledim_Win32TransportCtx*)malloc(sizeof(libledim_Win32TransportCtx));
	if (tmp_ctx == NULL) {
		CloseHandle(tmp_com_handle);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	tmp_ctx->com_handle = tmp_com_handle;
	tmp_ctx->last_set_timeout_ms = 0;

	PurgeComm(tmp_ctx->com_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);

	*arg_ret_ctx = tmp_ctx;

	return 0;
}

static void __close (void* arg_ctx) {
	libledim_Win32TransportCtx* subj_ctx = (libledim_Win32TransportCtx*)arg_ctx;

	if (subj_ctx == NULL) {
		return;
	}
	if (subj_ctx->com_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(subj_ctx->com_handle);
		subj_ctx->com_handle = INVALID_HANDLE_VALUE;
	}
	free(subj_ctx);

	return;
}

static void __flush (void* arg_ctx) {
	libledim_Win32TransportCtx* subj_ctx = (libledim_Win32TransportCtx*)arg_ctx;

	if (subj_ctx == NULL || subj_ctx->com_handle == INVALID_HANDLE_VALUE) {
		return;
	}
	PurgeComm(subj_ctx->com_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);

	return;
}

static uint32_t __read (void* arg_ctx, uint8_t* arg_rx_buf, uint32_t arg_num_bytes, uint32_t arg_timeout_ms) {
	libledim_Win32TransportCtx* subj_ctx = (libledim_Win32TransportCtx*)arg_ctx;
	DWORD tmp_num_read = 0;

	if (subj_ctx == NULL || subj_ctx->com_handle == INVALID_HANDLE_VALUE) {
		return 0;
	}
	if (__set_com_timeout(subj_ctx, arg_timeout_ms) != 0) {
		return 0;
	}

	// the read total timeout of the DCB makes ReadFile() return once the buffer is full or the
	// budget is spent, whichever comes first, so a short read means "the peer did not answer"
	if (!ReadFile(subj_ctx->com_handle, arg_rx_buf, arg_num_bytes, &tmp_num_read, NULL)) {
		return 0;
	}

	return (uint32_t)tmp_num_read;
}

static uint32_t __write (void* arg_ctx, const uint8_t* arg_tx_buf, uint32_t arg_num_bytes, uint32_t arg_timeout_ms) {
	libledim_Win32TransportCtx* subj_ctx = (libledim_Win32TransportCtx*)arg_ctx;
	DWORD tmp_num_written = 0;

	if (subj_ctx == NULL || subj_ctx->com_handle == INVALID_HANDLE_VALUE) {
		return 0;
	}
	if (__set_com_timeout(subj_ctx, arg_timeout_ms) != 0) {
		return 0;
	}

	if (!WriteFile(subj_ctx->com_handle, arg_tx_buf, arg_num_bytes, &tmp_num_written, NULL)) {
		return 0;
	}

	return (uint32_t)tmp_num_written;
}

const libledim_Transport libledim_Transport_Host = {
	.open = __open,
	.close = __close,
	.flush = __flush,
	.read = __read,
	.write = __write
};

#endif // _WIN32
