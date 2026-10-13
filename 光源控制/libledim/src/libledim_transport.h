#ifndef __libledim_transport_h
#define __libledim_transport_h

/*
* the transport: a serial port, opened and driven through the host api
*
* a transport instance is an opaque context plus the operations on it. the device layer only ever
* talks to this vtable, which is what keeps the frame exchange free of any platform ifdef, and
* what lets the tests swap in a loopback or a mock transport.
*
* all operations are synchronous and bounded in time: none of them blocks past its timeout.
*/

#include <stdbool.h>
#include <stdint.h>

typedef struct libledim_Transport {
	/*
	* open arg_portname, put it into raw 9600-8N1 mode and store the new context into *arg_ret_ctx
	* returns 0 on success, or the host level error code (errno / GetLastError()) on failure
	*/
	int (*open) (void** arg_ret_ctx, const char* arg_portname);

	// close a context, and release it
	void (*close) (void* arg_ctx);

	// drop whatever is buffered on either direction
	void (*flush) (void* arg_ctx);

	// read up to arg_num_bytes, waiting no longer than arg_timeout_ms in total
	// returns the number of bytes actually read, which may be short on timeout
	uint32_t (*read) (void* arg_ctx, uint8_t* arg_rx_buf, uint32_t arg_num_bytes, uint32_t arg_timeout_ms);

	// write up to arg_num_bytes within the total budget, returns the number of bytes actually written
	uint32_t (*write) (void* arg_ctx, const uint8_t* arg_tx_buf, uint32_t arg_num_bytes, uint32_t arg_timeout_ms);
} libledim_Transport;

// the transport of the host this library was built for
extern const libledim_Transport libledim_Transport_Host;

#endif // __libledim_transport_h
