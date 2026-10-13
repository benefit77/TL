#ifdef _WIN32

#include "libledim_portscan.h"

/*
* the win32 port walk: the serial ports of a windows host are listed in the registry, under
* HKLM\HARDWARE\DEVICEMAP\SERIALCOMM, where the kernel writes one value per port as it appears
*
* note: not compiled by the linux build this rewrite was developed on, it mirrors the registry
* walk of the original libledim.
*/

#include <stdlib.h>
#include <string.h>

#include <windows.h>

#define libledim_PortScan_MG_max_num_candidates 64
#define libledim_PortScan_MG_max_portname_len 64

void libledim_PortScan_CollectHostPortnames (libledim_PortnameList* arg_ret_list) {
	HKEY tmp_regkey = NULL;
	char tmp_regkey_name[MAX_PATH] = { 0 };
	char tmp_value_name[MAX_PATH] = { 0 };
	char tmp_value_data[MAX_PATH] = { 0 };

	char tmp_candidates[libledim_PortScan_MG_max_num_candidates][libledim_PortScan_MG_max_portname_len];
	uint32_t tmp_num_candidates = 0;

	arg_ret_list->names = NULL;
	arg_ret_list->count = 0;

	LSTATUS tmp_status = RegOpenKeyExA(
		HKEY_LOCAL_MACHINE,
		"HARDWARE\\DEVICEMAP\\SERIALCOMM",
		0,
		KEY_READ,
		&tmp_regkey
	);
	if (tmp_status != ERROR_SUCCESS) {
		return;
	}

	DWORD tmp_value_name_size = sizeof(tmp_value_name);
	DWORD tmp_value_data_size = sizeof(tmp_value_data);
	for (DWORD tmp_i = 0; tmp_num_candidates < libledim_PortScan_MG_max_num_candidates; tmp_i++) {
		tmp_value_name[0] = '\0';
		tmp_value_data[0] = '\0';
		tmp_value_name_size = sizeof(tmp_value_name);
		tmp_value_data_size = sizeof(tmp_value_data);

		tmp_status = RegEnumValueA(
			tmp_regkey,
			tmp_i,
			tmp_value_name,
			&tmp_value_name_size,
			NULL,
			NULL,
			(LPBYTE)tmp_value_data,
			&tmp_value_data_size
		);
		if (tmp_status == ERROR_NO_MORE_ITEMS) {
			break;
		}
		if (tmp_status != ERROR_SUCCESS) {
			continue;
		}
		if (strlen(tmp_value_data) >= libledim_PortScan_MG_max_portname_len) {
			continue;
		}

		// the data of the value is the portname itself, such as "COM3"
		strncpy(
			tmp_candidates[tmp_num_candidates],
			tmp_value_data,
			libledim_PortScan_MG_max_portname_len - 1
		);
		tmp_candidates[tmp_num_candidates][libledim_PortScan_MG_max_portname_len - 1] = '\0';
		tmp_num_candidates++;
	}

	RegCloseKey(tmp_regkey);

	if (tmp_num_candidates == 0) {
		return;
	}

	char** tmp_names = (char**)malloc(tmp_num_candidates * sizeof(char*));
	if (tmp_names == NULL) {
		return;
	}

	arg_ret_list->names = tmp_names;
	arg_ret_list->count = 0;

	for (uint32_t tmp_i = 0; tmp_i < tmp_num_candidates; tmp_i++) {
		size_t tmp_name_len = strlen(tmp_candidates[tmp_i]) + 1;
		tmp_names[tmp_i] = (char*)malloc(tmp_name_len);
		if (tmp_names[tmp_i] == NULL) {
			libledim_PortScan_FreeList(arg_ret_list);
			return;
		}
		memcpy(tmp_names[tmp_i], tmp_candidates[tmp_i], tmp_name_len);
		arg_ret_list->count++;
	}

	return;
}

#endif // _WIN32
