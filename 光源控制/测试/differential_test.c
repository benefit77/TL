/*
* the differential trace: one program, run twice
*
* it only ever calls the first generation interface (plus the port listing, which the rewritten
* library and the framework based port both expose), so the very same binary shape can be compiled
* against either library. what it prints, together with the frames the stand-in firmware logs, is
* the contract that must not have changed: run it against the old library, run it against this one,
* and compare the two transcripts.
*/

#include <libledim.h>

#include <stdio.h>
#include <string.h>

static void __report_bool (const char* arg_case, bool arg_value) {
	printf("%-32s %d\n", arg_case, arg_value ? 1 : 0);

	return;
}

static void __report_u8 (const char* arg_case, uint8_t arg_value) {
	printf("%-32s %u\n", arg_case, (unsigned)arg_value);

	return;
}

static void __report_bool_of (const char* arg_prefix, uint8_t arg_channel, bool arg_value) {
	char tmp_case[64] = { 0 };
	snprintf(tmp_case, sizeof(tmp_case), "%s ch%u", arg_prefix, (unsigned)arg_channel);
	__report_bool(tmp_case, arg_value);

	return;
}

static void __report_u8_of (const char* arg_prefix, uint8_t arg_channel, uint8_t arg_value) {
	char tmp_case[64] = { 0 };
	snprintf(tmp_case, sizeof(tmp_case), "%s ch%u", arg_prefix, (unsigned)arg_channel);
	__report_u8(tmp_case, arg_value);

	return;
}

int main (int argc, char* argv[]) {
	if (argc < 3) {
		fprintf(stderr, "usage: %s <port of the stand-in controller> <port that answers nothing>\n", argv[0]);
		return 2;
	}
	const char* subj_portname = argv[1];
	const char* subj_silent_portname = argv[2];

	// ---------------------------------------------------------------------------------------
	// before the controller is open
	// ---------------------------------------------------------------------------------------
	__report_u8("pre: get level ch1", libledim_GetChannelLuminanceLevel(1));
	__report_bool("pre: turn on ch1", libledim_EndisChannel(1, true));
	__report_bool("pre: set level ch1", libledim_SetChannelLuminanceLevel(1, 10));
	__report_bool("pre: set mode ch1", libledim_SetChannelMode(1, 0));
	__report_bool("pre: trigger flash ch1", libledim_TriggerChannelFlash(1));
	__report_bool("pre: set flash duration ch1", libledim_SetChannelFlashDuration(1, 10));
	printf("%-32s %s\n", "pre: portname", libledim_GetComPortname() == NULL ? "(null)" : libledim_GetComPortname());

	// ---------------------------------------------------------------------------------------
	// the host port listing
	// ---------------------------------------------------------------------------------------
	uint32_t tmp_num_ports = libledim_CountComPorts();
	printf("%-32s %u\n", "ports: count", tmp_num_ports);
	if (tmp_num_ports > 0) {
		printf("%-32s %s\n", "ports: first", libledim_PeekComPortByIdx(0));
		printf("%-32s %s\n", "ports: last", libledim_PeekComPortByIdx(tmp_num_ports - 1));
	}
	printf(
		"%-32s %s\n",
		"ports: out of range",
		libledim_PeekComPortByIdx(tmp_num_ports + 100) == NULL ? "(null)" : libledim_PeekComPortByIdx(tmp_num_ports + 100)
	);

	// ---------------------------------------------------------------------------------------
	// opening, and failing to open
	// ---------------------------------------------------------------------------------------
	__report_bool("open: not a serial port at all", libledim_Init("/dev/definitely-not-a-port"));
	__report_bool("open: a port that answers nothing", libledim_Init(subj_silent_portname));

	__report_bool("open: the controller", libledim_Init(subj_portname));
	printf("%-32s %s\n", "portname", libledim_GetComPortname() == NULL ? "(null)" : libledim_GetComPortname());
	__report_bool("open: again is a no-op", libledim_Init(subj_portname));

	// ---------------------------------------------------------------------------------------
	// arguments that are refused before the port is even touched
	// ---------------------------------------------------------------------------------------
	__report_u8("invalid: get level ch0", libledim_GetChannelLuminanceLevel(0));
	__report_u8("invalid: get level ch5", libledim_GetChannelLuminanceLevel(5));
	__report_u8("invalid: get level ch255", libledim_GetChannelLuminanceLevel(255));
	__report_bool("invalid: turn on ch0", libledim_EndisChannel(0, true));
	__report_bool("invalid: turn on ch5", libledim_EndisChannel(5, true));
	__report_bool("invalid: set level ch0", libledim_SetChannelLuminanceLevel(0, 1));
	__report_bool("invalid: set mode ch1 to 3", libledim_SetChannelMode(1, 3));
	__report_bool("invalid: set mode ch0 to 0", libledim_SetChannelMode(0, 0));

	// ---------------------------------------------------------------------------------------
	// the luminance read back
	// ---------------------------------------------------------------------------------------
	for (uint8_t tmp_ch = 1; tmp_ch <= libledim_MG_max_num_channels; tmp_ch++) {
		__report_u8_of("get level", tmp_ch, libledim_GetChannelLuminanceLevel(tmp_ch));
	}

	// ---------------------------------------------------------------------------------------
	// the channels, the stand-in controller refuses one of them
	// ---------------------------------------------------------------------------------------
	for (uint8_t tmp_ch = 1; tmp_ch <= libledim_MG_max_num_channels; tmp_ch++) {
		__report_bool_of("turn on", tmp_ch, libledim_EndisChannel(tmp_ch, true));
	}
	for (uint8_t tmp_ch = 1; tmp_ch <= libledim_MG_max_num_channels; tmp_ch++) {
		__report_bool_of("turn off", tmp_ch, libledim_EndisChannel(tmp_ch, false));
	}

	// ---------------------------------------------------------------------------------------
	// the luminance levels
	// ---------------------------------------------------------------------------------------
	__report_bool("set level ch1 to 0", libledim_SetChannelLuminanceLevel(1, 0));
	__report_bool("set level ch1 to 255", libledim_SetChannelLuminanceLevel(1, 255));
	__report_bool("set level ch2 to 128", libledim_SetChannelLuminanceLevel(2, 128));
	__report_u8("get level ch2 after the set", libledim_GetChannelLuminanceLevel(2));

	// ---------------------------------------------------------------------------------------
	// the modes
	// ---------------------------------------------------------------------------------------
	__report_bool("set mode ch2 to normal", libledim_SetChannelMode(2, 0));
	__report_bool("set mode ch2 to ext trigger", libledim_SetChannelMode(2, 1));
	__report_bool("set mode ch2 to flash", libledim_SetChannelMode(2, 2));

	// ---------------------------------------------------------------------------------------
	// the flash
	// ---------------------------------------------------------------------------------------
	__report_bool("set flash duration ch3 120", libledim_SetChannelFlashDuration(3, 120));
	__report_bool("trigger flash ch3", libledim_TriggerChannelFlash(3));
	__report_bool("set flash duration ch3 0", libledim_SetChannelFlashDuration(3, 0));

	// ---------------------------------------------------------------------------------------
	// and the same calls once the controller is closed again
	// ---------------------------------------------------------------------------------------
	libledim_Deinit();
	__report_u8("post: get level ch1", libledim_GetChannelLuminanceLevel(1));
	__report_bool("post: turn on ch1", libledim_EndisChannel(1, true));
	printf("%-32s %s\n", "post: portname", libledim_GetComPortname() == NULL ? "(null)" : libledim_GetComPortname());

	return 0;
}
