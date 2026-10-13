#ifndef __libledim_device_h
#define __libledim_device_h

#include "libledim.h"
#include "libledim_portscan.h"
#include "libledim_transport.h"

/*
* the device layer: one handle is one controller
*
* everything that knows about the controller lives here, and it all hangs off the handle, so two
* handles never share a single piece of mutable state (the host port walk, which is not device
* state, is the one exception, and it has its own cache in libledim_portscan.c)
*/

// long enough for "/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0", the longest name in practice
#define libledim_Device_MG_com_portname_buf_size 128

/*
* how long the initiate handshake with a controller may take while walking the ports of a host
* the walk is the one place where a short patience pays: an operator should not sit through the
* full transaction timeout for every port of a machine that has eight of them
*/
#define libledim_Device_MG_probe_timeout_ms 200

struct libledim_Device {
	const libledim_Transport* comp_transport;
	void* comp_transport_ctx;

	char ic_com_portname[libledim_Device_MG_com_portname_buf_size];
	uint32_t us_transaction_timeout_ms;

	libledim_Error prop_last_error;
	int prop_last_os_error;
};

// the error, as an english sentence
const char* libledim_Error_ToString (libledim_Error arg_error);

// allocate a handle that is bound to nothing yet (the counterpart of libledim_Device_Close())
libledim_Device* libledim_Device_Create (void);

// whether the handle currently holds an open port
bool libledim_Device_IsOpen (const libledim_Device* arg_device);

/*
* bind an already created handle to a controller
* arg_com_portname NULL means "walk the ports of this host"
* this is libledim_Device_Open() minus the handle creation, which is what the legacy facade needs:
* it has to set the timeout of the fresh handle *before* the port is probed, and the probe of an
* explicitly named port honours that timeout
*/
bool libledim_Device_Bind (libledim_Device* arg_device, const char* arg_com_portname);

#endif // __libledim_device_h
