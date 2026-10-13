#ifndef __libledim_log_h
#define __libledim_log_h

/*
* the logger: one callback, one level, and that is the whole story
*
* the line is formatted like the original library formatted its own ("I>  message", the level tag
* padded to 4 columns), so a log that used to be read by a person still reads the same way
*/

#include "libledim.h"

#if defined(__GNUC__) || defined(__clang__)
	// let the compiler check the format string against the arguments
	#define LIBLEDIM_PRINTF_LIKE(arg_fmt_idx, arg_first_arg_idx) __attribute__((format(printf, arg_fmt_idx, arg_first_arg_idx)))
#else
	#define LIBLEDIM_PRINTF_LIKE(arg_fmt_idx, arg_first_arg_idx)
#endif

// hand one formatted line to the mounted callback, dropping it when the level is below the minimum
void libledim_Log_Printf (libledim_LogLevel arg_level, const char* arg_fmt, ...)
	LIBLEDIM_PRINTF_LIKE(2, 3);

void libledim_Log_MountCallback (libledim_Log_Callback arg_callback);
void libledim_Log_SetMinLevel (libledim_LogLevel arg_level);

#endif // __libledim_log_h
