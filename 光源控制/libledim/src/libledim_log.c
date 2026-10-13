#include "libledim_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define libledim_Log_MG_max_line_len 512

static libledim_Log_Callback _fstatic_log_callback = NULL;
static libledim_LogLevel _fstatic_min_log_level = libledim_LogLevel_Info;

static const char* __level_tag_of (libledim_LogLevel arg_level) {
	switch (arg_level) {
	case libledim_LogLevel_Trace:
		return "T>";
	case libledim_LogLevel_Debug:
		return "D>";
	case libledim_LogLevel_Info:
		return "I>";
	case libledim_LogLevel_Warning:
		return "W>";
	case libledim_LogLevel_Error:
		return "E>";
	default:
		return "?>";
	}
}

void libledim_Log_Printf (libledim_LogLevel arg_level, const char* arg_fmt, ...) {
	static char __logline[libledim_Log_MG_max_line_len] = { 0 };

	if (_fstatic_log_callback == NULL) {
		return; // nobody is listening, do not even format the line
	}
	if (arg_level < _fstatic_min_log_level) {
		return;
	}

	va_list tmp_va_list;
	va_start(tmp_va_list, arg_fmt);
	int tmp_num_written = vsnprintf(
		__logline,
		sizeof(__logline),
		arg_fmt,
		tmp_va_list
	);
	va_end(tmp_va_list);

	if (tmp_num_written < 0) {
		return;
	}

	// the tag is padded to 4 columns, so that the messages line up no matter the level
	char tmp_tagged_line[libledim_Log_MG_max_line_len] = { 0 };
	snprintf(
		tmp_tagged_line,
		sizeof(tmp_tagged_line),
		"%-*s%.*s",
		4,
		__level_tag_of(arg_level),
		// the line is truncated rather than dropped, an oversized message is still worth reading
		(int)(sizeof(tmp_tagged_line) - 5),
		__logline
	);

	_fstatic_log_callback(arg_level, tmp_tagged_line);

	return;
}

void libledim_Log_MountCallback (libledim_Log_Callback arg_callback) {
	_fstatic_log_callback = arg_callback;

	return;
}

void libledim_Log_SetMinLevel (libledim_LogLevel arg_level) {
	_fstatic_min_log_level = arg_level;

	return;
}

void libledim_LogToStderr (libledim_LogLevel arg_level, const char* arg_line) {
	(void)arg_level;
	fputs(arg_line, stderr);
	fflush(stderr);

	return;
}
