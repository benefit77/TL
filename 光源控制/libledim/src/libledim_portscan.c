#include "libledim_portscan.h"

#include "libledim.h"

#include <stdlib.h>

static libledim_PortnameList _fstatic_cached_portnames = { NULL, 0 };

void libledim_PortScan_FreeList (libledim_PortnameList* arg_list) {
	if (arg_list == NULL) {
		return;
	}

	for (uint32_t tmp_i = 0; tmp_i < arg_list->count; tmp_i++) {
		free(arg_list->names[tmp_i]);
	}
	free(arg_list->names);

	arg_list->names = NULL;
	arg_list->count = 0;

	return;
}

void libledim_PortScan_InvalidateCache (void) {
	libledim_PortScan_FreeList(&_fstatic_cached_portnames);

	return;
}

uint32_t libledim_CountComPorts (void) {
	// walk the host again on every call, so that a device plugged in (or pulled out) in between is
	// reflected, and hand the fresh answer out through the cache
	libledim_PortScan_InvalidateCache();
	libledim_PortScan_CollectHostPortnames(&_fstatic_cached_portnames);

	return _fstatic_cached_portnames.count;
}

const char* libledim_PeekComPortByIdx (uint32_t arg_idx) {
	if (arg_idx >= _fstatic_cached_portnames.count) {
		return NULL;
	}

	return _fstatic_cached_portnames.names[arg_idx];
}
