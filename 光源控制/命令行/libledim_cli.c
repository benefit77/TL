/*
* libledim-clidemo, the command line front end of libledim
*
* a thin wrapper around the library, with nothing clever in it: it parses the arguments, calls one
* library function, and reports what came back. when something fails it also reports why, which is
* the whole point of the error reporting the rewrite added.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libledim.h>

#define CLI_MG_default_transaction_timeout_ms 500

static void print_usage (const char* arg_prog_name) {
	fprintf(stderr,
		"libledim-clidemo: command line front end of libledim (light source control)\n"
		"\n"
		"usage: %s [options] <command> [args]\n"
		"\n"
		"options:\n"
		"  -p, --port <portname>   serial port to use, such as /dev/ttyUSB0\n"
		"                          when omitted, the host is walked for a port that answers\n"
		"  -t, --timeout <ms>      transaction timeout in milliseconds (default %d)\n"
		"  -v, --verbose           log at debug level\n"
		"  -h, --help              this message\n"
		"\n"
		"commands:\n"
		"  list                    list the serial ports found on this host\n"
		"  on <ch>                 turn channel <ch> on\n"
		"  off <ch>                turn channel <ch> off\n"
		"  level <ch> <0..255>     set the luminance level of channel <ch> (and read it back)\n"
		"  get <ch>                read back the luminance level of channel <ch>\n"
		"  mode <ch> <0|1|2>       set the mode of channel <ch>\n"
		"                          0 normal (always on), 1 external trigger, 2 flash\n"
		"  flash <ch> [0..255]     put channel <ch> into flash mode, set the flash duration\n"
		"                          (default 100ms) and trigger one flash\n"
		"  selftest                exercise every channel: toggle it, sweep the luminance level,\n"
		"                          then flash with several durations\n"
		"\n"
		"channels are numbered 1 to %d\n"
		"\n",
		arg_prog_name,
		CLI_MG_default_transaction_timeout_ms,
		libledim_MG_max_num_channels
	);

	return;
}

// returns true and stores the parsed number into arg_ret_value, false when arg_str is not a plain integer
static bool parse_int (const char* arg_str, long arg_min, long arg_max, long* arg_ret_value) {
	if (arg_str == NULL || arg_str[0] == '\0') {
		return false;
	}

	char* tmp_end = NULL;
	long tmp_value = strtol(arg_str, &tmp_end, 10);
	if (tmp_end == arg_str || *tmp_end != '\0') {
		return false;
	}
	if (tmp_value < arg_min || tmp_value > arg_max) {
		return false;
	}

	*arg_ret_value = tmp_value;

	return true;
}

// returns a channel order (1..max), or 0 when arg_str is not a valid one
static int parse_channel (const char* arg_str) {
	long tmp_channel = 0;
	if (!parse_int(arg_str, 1, libledim_MG_max_num_channels, &tmp_channel)) {
		return 0;
	}

	return (int)tmp_channel;
}

// returns the value (0..255), or -1 when arg_str is not a valid one
static int parse_u8 (const char* arg_str) {
	long tmp_value = 0;
	if (!parse_int(arg_str, 0, 255, &tmp_value)) {
		return -1;
	}

	return (int)tmp_value;
}

// report the outcome of one command, with the reason when it went wrong
static void report_outcome (const char* arg_action, int arg_channel, bool arg_tof) {
	if (arg_tof) {
		printf("%s: channel %d, ok\n", arg_action, arg_channel);
		return;
	}

	printf(
		"%s: channel %d, FAILED (%s)\n",
		arg_action,
		arg_channel,
		libledim_GetLastErrorString()
	);

	return;
}

static int cmd_list (void) {
	// listing the ports of a host is a facility of its own, it does not need a controller to be open
	uint32_t tmp_num_ports = libledim_CountComPorts();

	if (tmp_num_ports == 0) {
		printf("no serial port found\n");
		return 1;
	}

	printf("%u serial port(s) found:\n", tmp_num_ports);
	for (uint32_t tmp_i = 0; tmp_i < tmp_num_ports; tmp_i++) {
		printf("  [%u] %s\n", tmp_i, libledim_PeekComPortByIdx(tmp_i));
	}

	return 0;
}

static int selftest_all_channels (void) {
	int tmp_num_failures = 0;

	// pass 1: put every channel into normal (always on) mode and toggle it on and off a couple of times
	for (uint8_t tmp_ch = 1; tmp_ch <= libledim_MG_max_num_channels; tmp_ch++) {
		if (!libledim_SetChannelMode(tmp_ch, 0)) {
			tmp_num_failures++;
		}

		for (int tmp_i = 0; tmp_i < 2; tmp_i++) {
			if (!libledim_EndisChannel(tmp_ch, true)) {
				tmp_num_failures++;
			}
			if (!libledim_EndisChannel(tmp_ch, false)) {
				tmp_num_failures++;
			}
		}
		printf("channel %u: normal mode toggled\n", tmp_ch);
	}

	// pass 2: sweep the luminance level of every channel
	for (uint8_t tmp_ch = 1; tmp_ch <= libledim_MG_max_num_channels; tmp_ch++) {
		libledim_EndisChannel(tmp_ch, true);

		for (int tmp_level = 0; tmp_level <= 255; tmp_level += 50) {
			if (!libledim_SetChannelLuminanceLevel(tmp_ch, (uint8_t)tmp_level)) {
				tmp_num_failures++;
			}
			printf(
				"channel %u: level %3d set, read back %u\n",
				tmp_ch,
				tmp_level,
				libledim_GetChannelLuminanceLevel(tmp_ch)
			);
		}

		libledim_EndisChannel(tmp_ch, false);
	}

	// pass 3: flash every channel with a few durations
	for (uint8_t tmp_ch = 1; tmp_ch <= libledim_MG_max_num_channels; tmp_ch++) {
		if (!libledim_SetChannelMode(tmp_ch, 2)) {
			tmp_num_failures++;
		}

		for (int tmp_duration = 50; tmp_duration <= 250; tmp_duration += 50) {
			if (!libledim_SetChannelFlashDuration(tmp_ch, (uint8_t)tmp_duration)) {
				tmp_num_failures++;
			}
			if (!libledim_TriggerChannelFlash(tmp_ch)) {
				tmp_num_failures++;
			}
			printf("channel %u: flashed for %d ms\n", tmp_ch, tmp_duration);
		}
	}

	printf("\n");
	if (tmp_num_failures == 0) {
		printf("selftest: all transactions succeeded\n");
	} else {
		printf("selftest: %d transaction(s) failed\n", tmp_num_failures);
	}

	return (tmp_num_failures == 0) ? 0 : 1;
}

int main (int argc, char* argv[]) {
	const char* subj_com_portname = NULL;
	uint32_t subj_timeout_ms = CLI_MG_default_transaction_timeout_ms;
	bool subj_is_verbose = false;

	int tmp_argi = 1;
	for (; tmp_argi < argc; tmp_argi++) {
		const char* tmp_arg = argv[tmp_argi];

		if (strcmp(tmp_arg, "-h") == 0 || strcmp(tmp_arg, "--help") == 0) {
			print_usage(argv[0]);
			return 0;
		} else if (strcmp(tmp_arg, "-v") == 0 || strcmp(tmp_arg, "--verbose") == 0) {
			subj_is_verbose = true;
		} else if (strcmp(tmp_arg, "-p") == 0 || strcmp(tmp_arg, "--port") == 0) {
			if (tmp_argi + 1 >= argc) {
				fprintf(stderr, "error: %s needs an argument\n", tmp_arg);
				return 2;
			}
			tmp_argi++;
			subj_com_portname = argv[tmp_argi];
		} else if (strcmp(tmp_arg, "-t") == 0 || strcmp(tmp_arg, "--timeout") == 0) {
			if (tmp_argi + 1 >= argc) {
				fprintf(stderr, "error: %s needs an argument\n", tmp_arg);
				return 2;
			}
			tmp_argi++;
			long tmp_timeout_ms = 0;
			if (!parse_int(argv[tmp_argi], 1, 600000, &tmp_timeout_ms)) {
				fprintf(stderr, "error: invalid timeout '%s'\n", argv[tmp_argi]);
				return 2;
			}
			subj_timeout_ms = (uint32_t)tmp_timeout_ms;
		} else if (tmp_arg[0] == '-' && tmp_arg[1] != '\0') {
			fprintf(stderr, "error: unknown option '%s'\n", tmp_arg);
			print_usage(argv[0]);
			return 2;
		} else {
			break; // the command begins
		}
	}

	const char* subj_command = NULL;
	if (tmp_argi < argc) {
		subj_command = argv[tmp_argi];
		tmp_argi++;
	}

	if (subj_command == NULL) {
		print_usage(argv[0]);
		return 2;
	}

	if (strcmp(subj_command, "list") == 0) {
		return cmd_list();
	}

	// everything below talks to the controller, so let the log lines through to stderr
	libledim_SetLogCallback(libledim_LogToStderr);
	libledim_SetMinLogLevel(subj_is_verbose ? libledim_LogLevel_Debug : libledim_LogLevel_Info);

	libledim_SetTransactionTimeout(subj_timeout_ms);

	if (!libledim_Init(subj_com_portname)) {
		fprintf(
			stderr,
			"error: libledim_Init(%s) failed: %s",
			subj_com_portname == NULL ? "NULL, host walk" : subj_com_portname,
			libledim_GetLastErrorString()
		);
		if (libledim_GetLastOsError() != 0) {
			fprintf(stderr, " (os error %d)", libledim_GetLastOsError());
		}
		fprintf(stderr, "\n");
		return 1;
	}
	printf("libledim bound to %s\n", libledim_GetComPortname());

	int tmp_retcode = 0;

	if (strcmp(subj_command, "selftest") == 0) {
		tmp_retcode = selftest_all_channels();
	} else if (strcmp(subj_command, "on") == 0 || strcmp(subj_command, "off") == 0) {
		bool tmp_turn_on = (strcmp(subj_command, "on") == 0);
		int tmp_channel = parse_channel((tmp_argi < argc) ? argv[tmp_argi] : NULL);
		if (tmp_channel == 0) {
			fprintf(stderr, "error: %s needs a channel between 1 and %d\n", subj_command, libledim_MG_max_num_channels);
			libledim_Deinit();
			return 2;
		}
		bool tmp_tof = libledim_EndisChannel((uint8_t)tmp_channel, tmp_turn_on);
		report_outcome(tmp_turn_on ? "turn on" : "turn off", tmp_channel, tmp_tof);
		tmp_retcode = tmp_tof ? 0 : 1;
	} else if (strcmp(subj_command, "level") == 0) {
		int tmp_channel = parse_channel((tmp_argi < argc) ? argv[tmp_argi] : NULL);
		int tmp_level = parse_u8((tmp_argi + 1 < argc) ? argv[tmp_argi + 1] : NULL);
		if (tmp_channel == 0 || tmp_level < 0) {
			fprintf(stderr, "error: level needs a channel (1..%d) and a level (0..255)\n", libledim_MG_max_num_channels);
			libledim_Deinit();
			return 2;
		}
		bool tmp_tof = libledim_SetChannelLuminanceLevel((uint8_t)tmp_channel, (uint8_t)tmp_level);
		if (tmp_tof) {
			printf(
				"set level: channel %d, level %d, ok (read back %u)\n",
				tmp_channel,
				tmp_level,
				libledim_GetChannelLuminanceLevel((uint8_t)tmp_channel)
			);
		} else {
			report_outcome("set level", tmp_channel, false);
		}
		tmp_retcode = tmp_tof ? 0 : 1;
	} else if (strcmp(subj_command, "get") == 0) {
		int tmp_channel = parse_channel((tmp_argi < argc) ? argv[tmp_argi] : NULL);
		if (tmp_channel == 0) {
			fprintf(stderr, "error: get needs a channel between 1 and %d\n", libledim_MG_max_num_channels);
			libledim_Deinit();
			return 2;
		}
		uint8_t tmp_level = libledim_GetChannelLuminanceLevel((uint8_t)tmp_channel);
		if (libledim_GetLastError() != libledim_Error_None) {
			printf(
				"channel %d: no level to read (%s)\n",
				tmp_channel,
				libledim_GetLastErrorString()
			);
			tmp_retcode = 1;
		} else {
			printf("channel %d: luminance level %u\n", tmp_channel, tmp_level);
		}
	} else if (strcmp(subj_command, "mode") == 0) {
		int tmp_channel = parse_channel((tmp_argi < argc) ? argv[tmp_argi] : NULL);
		int tmp_mode = parse_u8((tmp_argi + 1 < argc) ? argv[tmp_argi + 1] : NULL);
		if (tmp_channel == 0 || tmp_mode < 0 || tmp_mode > 2) {
			fprintf(stderr, "error: mode needs a channel (1..%d) and a mode (0 normal, 1 external trigger, 2 flash)\n", libledim_MG_max_num_channels);
			libledim_Deinit();
			return 2;
		}
		bool tmp_tof = libledim_SetChannelMode((uint8_t)tmp_channel, (uint8_t)tmp_mode);
		report_outcome("set mode", tmp_channel, tmp_tof);
		tmp_retcode = tmp_tof ? 0 : 1;
	} else if (strcmp(subj_command, "flash") == 0) {
		int tmp_channel = parse_channel((tmp_argi < argc) ? argv[tmp_argi] : NULL);
		int tmp_duration = 100; // a sensible default
		if (tmp_argi + 1 < argc) {
			tmp_duration = parse_u8(argv[tmp_argi + 1]);
		}
		if (tmp_channel == 0 || tmp_duration < 0) {
			fprintf(stderr, "error: flash needs a channel (1..%d) and an optional duration (0..255ms)\n", libledim_MG_max_num_channels);
			libledim_Deinit();
			return 2;
		}
		bool tmp_tof = libledim_SetChannelMode((uint8_t)tmp_channel, 2); // flash mode
		if (tmp_tof) {
			tmp_tof = libledim_SetChannelFlashDuration((uint8_t)tmp_channel, (uint8_t)tmp_duration);
		}
		if (tmp_tof) {
			tmp_tof = libledim_TriggerChannelFlash((uint8_t)tmp_channel);
		}
		if (tmp_tof) {
			printf("flash: channel %d, %d ms, ok\n", tmp_channel, tmp_duration);
		} else {
			report_outcome("flash", tmp_channel, false);
		}
		tmp_retcode = tmp_tof ? 0 : 1;
	} else {
		fprintf(stderr, "error: unknown command '%s'\n", subj_command);
		print_usage(argv[0]);
		libledim_Deinit();
		return 2;
	}

	libledim_Deinit();

	return tmp_retcode;
}
