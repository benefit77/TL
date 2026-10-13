#ifndef __libledim_portscan_h
#define __libledim_portscan_h

#include <stdbool.h>
#include <stdint.h>

/*
* the serial ports of the host
*
* two layers live here:
*   - the raw enumeration, which is the only host specific part (POSIX /dev walk, win32 registry)
*   - a cache on top of it, which is what libledim_CountComPorts()/libledim_PeekComPortByIdx()
*     hand out (the alternative, returning a malloc()ed array to the caller, would push the
*     ownership of library memory onto the caller, which is a poor idea across a DLL boundary)
*/

typedef struct libledim_PortnameList {
	char** names;
	uint32_t count;
} libledim_PortnameList;

// enumerate the ports of this host, allocating the list (an empty list is a valid answer)
void libledim_PortScan_CollectHostPortnames (libledim_PortnameList* arg_ret_list);

// release a list produced by libledim_PortScan_CollectHostPortnames()
void libledim_PortScan_FreeList (libledim_PortnameList* arg_list);

// drop the cache behind libledim_CountComPorts()/libledim_PeekComPortByIdx()
void libledim_PortScan_InvalidateCache (void);

#endif // __libledim_portscan_h
