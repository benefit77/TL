#include "libledim_transport.h"

/*
* the termios transport, for linux (and for any other unix that speaks termios, macos included)
*
* the port is opened non blocking, so that opening a port whose carrier is not asserted, or a port
* that is not a serial port at all, cannot hang the process (which matters during a port walk).
* the waiting is then done with select(): VMIN/VTIME are left at 0 and the timeouts live in the
* read/write loops, where they belong.
*/

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

typedef struct libledim_PosixTransportCtx {
	int fd;
} libledim_PosixTransportCtx;

static uint64_t __now_ms (void) {
	struct timespec tmp_timespec = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &tmp_timespec);

	return (uint64_t)tmp_timespec.tv_sec * 1000ULL + (uint64_t)(tmp_timespec.tv_nsec / 1000000L);
}

static void __timeval_of_ms (uint32_t arg_timeout_ms, struct timeval* arg_ret_timeval) {
	arg_ret_timeval->tv_sec = (time_t)(arg_timeout_ms / 1000U);
	arg_ret_timeval->tv_usec = (suseconds_t)((arg_timeout_ms % 1000U) * 1000U);

	return;
}

static bool __wait_readable (int arg_fd, uint32_t arg_timeout_ms) {
	fd_set tmp_read_fdset;
	struct timeval tmp_timeval = { 0 };

	FD_ZERO(&tmp_read_fdset);
	FD_SET(arg_fd, &tmp_read_fdset);
	__timeval_of_ms(arg_timeout_ms, &tmp_timeval);

	// select() answers -1 on error, 0 on timeout, >0 on ready
	return (select(arg_fd + 1, &tmp_read_fdset, NULL, NULL, &tmp_timeval) > 0);
}

static bool __wait_writable (int arg_fd, uint32_t arg_timeout_ms) {
	fd_set tmp_write_fdset;
	struct timeval tmp_timeval = { 0 };

	FD_ZERO(&tmp_write_fdset);
	FD_SET(arg_fd, &tmp_write_fdset);
	__timeval_of_ms(arg_timeout_ms, &tmp_timeval);

	return (select(arg_fd + 1, NULL, &tmp_write_fdset, NULL, &tmp_timeval) > 0);
}

static int __open (void** arg_ret_ctx, const char* arg_portname) {
	libledim_PosixTransportCtx* tmp_ctx = NULL;
	struct termios tmp_termios = { 0 };

	int tmp_fd = open(arg_portname, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (tmp_fd < 0) {
		return errno;
	}

	if (tcgetattr(tmp_fd, &tmp_termios) != 0) {
		int tmp_errcode = errno;
		close(tmp_fd);
		return tmp_errcode;
	}

	// speed: the firmware speaks 9600 baud
	cfsetispeed(&tmp_termios, B9600);
	cfsetospeed(&tmp_termios, B9600);

	// 8 data bits, no parity, 1 stop bit
	tmp_termios.c_cflag &= (tcflag_t)~CSIZE;
	tmp_termios.c_cflag |= CS8;
	tmp_termios.c_cflag &= (tcflag_t)~(PARENB | PARODD);
	tmp_termios.c_cflag &= (tcflag_t)~CSTOPB;

	// no flow control at all (neither rts/cts nor xon/xoff)
	tmp_termios.c_cflag &= (tcflag_t)~CRTSCTS;
	tmp_termios.c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);

	// ignore modem control lines, enable the receiver
	tmp_termios.c_cflag |= (CLOCAL | CREAD);

	// raw mode: no canonical input, no echo, no signals, no cr/lf translation
	tmp_termios.c_lflag &= (tcflag_t)~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN);
	tmp_termios.c_iflag &= (tcflag_t)~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
	tmp_termios.c_oflag &= (tcflag_t)~OPOST;

	// the timeouts are implemented with select(), see above
	tmp_termios.c_cc[VMIN] = 0;
	tmp_termios.c_cc[VTIME] = 0;

	if (tcsetattr(tmp_fd, TCSANOW, &tmp_termios) != 0) {
		int tmp_errcode = errno;
		close(tmp_fd);
		return tmp_errcode;
	}

	// whatever the peer sent before the port was configured is of no interest
	tcflush(tmp_fd, TCIOFLUSH);

	tmp_ctx = (libledim_PosixTransportCtx*)malloc(sizeof(libledim_PosixTransportCtx));
	if (tmp_ctx == NULL) {
		close(tmp_fd);
		return ENOMEM;
	}
	tmp_ctx->fd = tmp_fd;
	*arg_ret_ctx = tmp_ctx;

	return 0;
}

static void __close (void* arg_ctx) {
	libledim_PosixTransportCtx* subj_ctx = (libledim_PosixTransportCtx*)arg_ctx;

	if (subj_ctx == NULL) {
		return;
	}
	if (subj_ctx->fd >= 0) {
		close(subj_ctx->fd);
		subj_ctx->fd = -1;
	}
	free(subj_ctx);

	return;
}

static void __flush (void* arg_ctx) {
	libledim_PosixTransportCtx* subj_ctx = (libledim_PosixTransportCtx*)arg_ctx;

	if (subj_ctx == NULL || subj_ctx->fd < 0) {
		return;
	}
	tcflush(subj_ctx->fd, TCIOFLUSH);

	return;
}

static uint32_t __read (void* arg_ctx, uint8_t* arg_rx_buf, uint32_t arg_num_bytes, uint32_t arg_timeout_ms) {
	libledim_PosixTransportCtx* subj_ctx = (libledim_PosixTransportCtx*)arg_ctx;
	uint32_t tmp_num_read = 0;

	if (subj_ctx == NULL || subj_ctx->fd < 0) {
		return 0;
	}

	uint64_t tmp_deadline_ms = __now_ms() + (uint64_t)arg_timeout_ms;
	while (tmp_num_read < arg_num_bytes) {
		uint64_t tmp_now_ms = __now_ms();
		if (tmp_now_ms >= tmp_deadline_ms) {
			break; // out of time
		}
		if (!__wait_readable(subj_ctx->fd, (uint32_t)(tmp_deadline_ms - tmp_now_ms))) {
			break; // out of time, or select() failed
		}

		ssize_t tmp_num_read_this_round = read(
			subj_ctx->fd,
			arg_rx_buf + tmp_num_read,
			(size_t)(arg_num_bytes - tmp_num_read)
		);
		if (tmp_num_read_this_round > 0) {
			tmp_num_read += (uint32_t)tmp_num_read_this_round;
		} else if (tmp_num_read_this_round < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
			continue; // spurious wakeup, a non blocking port with nothing to give
		} else {
			break; // hard error, or the peer went away
		}
	}

	return tmp_num_read;
}

static uint32_t __write (void* arg_ctx, const uint8_t* arg_tx_buf, uint32_t arg_num_bytes, uint32_t arg_timeout_ms) {
	libledim_PosixTransportCtx* subj_ctx = (libledim_PosixTransportCtx*)arg_ctx;
	uint32_t tmp_num_written = 0;

	if (subj_ctx == NULL || subj_ctx->fd < 0) {
		return 0;
	}

	uint64_t tmp_deadline_ms = __now_ms() + (uint64_t)arg_timeout_ms;
	while (tmp_num_written < arg_num_bytes) {
		uint64_t tmp_now_ms = __now_ms();
		if (tmp_now_ms >= tmp_deadline_ms) {
			break; // out of time
		}
		if (!__wait_writable(subj_ctx->fd, (uint32_t)(tmp_deadline_ms - tmp_now_ms))) {
			break; // out of time, or select() failed
		}

		ssize_t tmp_num_written_this_round = write(
			subj_ctx->fd,
			arg_tx_buf + tmp_num_written,
			(size_t)(arg_num_bytes - tmp_num_written)
		);
		if (tmp_num_written_this_round > 0) {
			tmp_num_written += (uint32_t)tmp_num_written_this_round;
		} else if (tmp_num_written_this_round < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
			continue;
		} else {
			break; // hard error
		}
	}

	return tmp_num_written;
}

const libledim_Transport libledim_Transport_Host = {
	.open = __open,
	.close = __close,
	.flush = __flush,
	.read = __read,
	.write = __write
};
