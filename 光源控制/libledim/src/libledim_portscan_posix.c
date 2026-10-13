#include "libledim_portscan.h"

/*
* the POSIX port walk: look through /dev for the character device nodes a serial port shows up as
*
* a device node name is not proof of anything on its own, the kernel keeps plenty of dangling
* entries around (every /dev/ttyS<N> of a machine that has no such uart, for one), so a node is
* only kept when it is a character device and the kernel backs it with an actual device
* (/sys/class/tty/<name>/device).
*/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define libledim_PortScan_MG_max_num_candidates 64
// the whole "/dev/<node>" path, as open(2) will receive it
#define libledim_PortScan_MG_max_portname_len 64
// the bare device node name (ttyUSB0 and the like), a name this long is not a serial port
#define libledim_PortScan_MG_max_node_name_len 48
// "/sys/class/tty/" + "/device" + the trailing '\0'
#define libledim_PortScan_MG_sysfs_affix_len 26

// ttyUSB -> usb to serial bridge (ftdi/ch340/cp210x/pl2303/...)
// ttyACM -> usb cdc-acm
// ttyAMA -> pl011 uart of arm boards, such as the raspberry pi
// ttyPS  -> xilinx zynq ps uart
// ttyS   -> 16550 compatible uart
// rfcomm -> bluetooth serial profile
static const char* const __serial_prefixes[] = {
	"ttyUSB",
	"ttyACM",
	"ttyAMA",
	"ttyPS",
	"ttyS",
	"rfcomm",
	NULL
};

static bool __name_is_candidate (const char* arg_name) {
	for (int tmp_i = 0; __serial_prefixes[tmp_i] != NULL; tmp_i++) {
		if (strncmp(arg_name, __serial_prefixes[tmp_i], strlen(__serial_prefixes[tmp_i])) == 0) {
			return true;
		}
	}

	return false;
}

static bool __node_is_backed_by_a_device (const char* arg_node_name) {
	char tmp_sysfs_path[libledim_PortScan_MG_max_node_name_len + libledim_PortScan_MG_sysfs_affix_len] = { 0 };
	struct stat tmp_stat = { 0 };

	snprintf(
		tmp_sysfs_path,
		sizeof(tmp_sysfs_path),
		"/sys/class/tty/%.*s/device",
		(int)libledim_PortScan_MG_max_node_name_len,
		arg_node_name
	);

	return (stat(tmp_sysfs_path, &tmp_stat) == 0);
}

static int __compare_portnames (const void* arg_lhs, const void* arg_rhs) {
	return strcmp((const char*)arg_lhs, (const char*)arg_rhs);
}

void libledim_PortScan_CollectHostPortnames (libledim_PortnameList* arg_ret_list) {
	char tmp_candidates[libledim_PortScan_MG_max_num_candidates][libledim_PortScan_MG_max_portname_len];
	uint32_t tmp_num_candidates = 0;

	arg_ret_list->names = NULL;
	arg_ret_list->count = 0;

	DIR* tmp_dev_dir = opendir("/dev");
	if (tmp_dev_dir == NULL) {
		return;
	}

	struct dirent* tmp_dirent = NULL;
	while ((tmp_dirent = readdir(tmp_dev_dir)) != NULL) {
		if (tmp_num_candidates == libledim_PortScan_MG_max_num_candidates) {
			break;
		}
		if (!__name_is_candidate(tmp_dirent->d_name)) {
			continue;
		}
		if (strlen(tmp_dirent->d_name) >= libledim_PortScan_MG_max_node_name_len) {
			continue;
		}

		char tmp_dev_path[libledim_PortScan_MG_max_portname_len] = { 0 };
		snprintf(
			tmp_dev_path,
			sizeof(tmp_dev_path),
			"/dev/%.*s",
			(int)(libledim_PortScan_MG_max_node_name_len - 1),
			tmp_dirent->d_name
		);

		struct stat tmp_stat = { 0 };
		if (stat(tmp_dev_path, &tmp_stat) != 0 || !S_ISCHR(tmp_stat.st_mode)) {
			continue;
		}
		if (!__node_is_backed_by_a_device(tmp_dirent->d_name)) {
			continue;
		}

		// the name was already proven shorter than the buffer above, so this copy cannot overflow
		memcpy(
			tmp_candidates[tmp_num_candidates],
			tmp_dev_path,
			strlen(tmp_dev_path) + 1
		);
		tmp_num_candidates++;
	}
	closedir(tmp_dev_dir);

	if (tmp_num_candidates == 0) {
		return;
	}

	// readdir() order is whatever the filesystem feels like, sorting keeps the walk repeatable
	qsort(
		tmp_candidates,
		tmp_num_candidates,
		libledim_PortScan_MG_max_portname_len,
		__compare_portnames
	);

	char** tmp_names = (char**)malloc(tmp_num_candidates * sizeof(char*));
	if (tmp_names == NULL) {
		return;
	}

	for (uint32_t tmp_i = 0; tmp_i < tmp_num_candidates; tmp_i++) {
		size_t tmp_name_len = strlen(tmp_candidates[tmp_i]) + 1;
		tmp_names[tmp_i] = (char*)malloc(tmp_name_len);
		if (tmp_names[tmp_i] != NULL) {
			memcpy(tmp_names[tmp_i], tmp_candidates[tmp_i], tmp_name_len);
		}
		if (tmp_names[tmp_i] == NULL) {
			arg_ret_list->names = tmp_names;
			arg_ret_list->count = tmp_i;
			libledim_PortScan_FreeList(arg_ret_list);
			return;
		}
	}

	arg_ret_list->names = tmp_names;
	arg_ret_list->count = tmp_num_candidates;

	return;
}
