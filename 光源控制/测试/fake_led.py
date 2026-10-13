#!/usr/bin/env python3
"""A stand-in for the light source controller, for exercising libledim without hardware.

It owns the master side of a pty pair, so a library that opens the slave side talks to it exactly
like it would talk to a real controller:

  request (8 bytes): '$' opcode channel '0' arg_hi arg_lo cksum_hi cksum_lo
  reply:             8 bytes carrying the luminance level, for opcode '4'
                     1 byte, '$' accepted or '&' refused, for every other opcode

Every frame is written to the log file, as "rx frame b'...'" and "tx frame b'...'", which is what
makes the differential test able to compare the bytes this library puts on the wire against the
bytes the first generation of libledim put on the wire.
"""

import argparse
import errno
import os
import pty
import sys
import time

OPCODE_TURN_ON = ord('1')
OPCODE_TURN_OFF = ord('2')
OPCODE_SET_LEVEL = ord('3')
OPCODE_GET_LEVEL = ord('4')
OPCODE_TRIGGER_FLASH = ord('7')
OPCODE_SET_MODE = ord('8')
OPCODE_SET_FLASH_DURATION = ord('9')

HEXCHARS = b'0123456789ABCDEF'


def hexval(ch):
    if 48 <= ch <= 57:
        return ch - 48
    if 65 <= ch <= 70:
        return ch - 55
    if 97 <= ch <= 102:
        return ch - 87
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port-file', required=True,
                        help='the slave side portname is written here, as soon as it is ready')
    parser.add_argument('--log-file', required=True, help='every frame read and written lands here')
    parser.add_argument('--level', type=lambda s: int(s, 0), default=0xAB,
                        help='the luminance level the controller reports (default 0xAB)')
    parser.add_argument('--reject-channel', type=int, default=4,
                        help='commands for this channel are refused with "&" (default 4)')
    parser.add_argument('--silent', action='store_true',
                        help='open the port and answer nothing at all, a port that holds no controller')
    args = parser.parse_args()

    master, slave = pty.openpty()
    portname = os.ttyname(slave)
    with open(args.port_file, 'w') as port_file:
        port_file.write(portname)

    log = open(args.log_file, 'a', buffering=1)
    log.write('listening on %s\n' % portname)

    level = args.level
    pending = b''

    while True:
        try:
            chunk = os.read(master, 256)
        except OSError as exc:
            if exc.errno == errno.EIO:
                # the peer closed the port, that is normal in between two runs of a test
                time.sleep(0.01)
                continue
            raise

        if not chunk:
            time.sleep(0.01)
            continue

        log.write('rx chunk %r\n' % chunk)
        if args.silent:
            continue

        pending += chunk
        while len(pending) >= 8:
            frame, pending = pending[:8], pending[8:]
            opcode = frame[1]
            channel = frame[2] - 48
            arg = (hexval(frame[4]) << 4) | hexval(frame[5])

            if channel == args.reject_channel and opcode in (
                    OPCODE_TURN_ON, OPCODE_TURN_OFF, OPCODE_SET_LEVEL,
                    OPCODE_TRIGGER_FLASH, OPCODE_SET_MODE, OPCODE_SET_FLASH_DURATION):
                reply = b'&'
            elif opcode == OPCODE_GET_LEVEL:
                # the library reads the level off the 5th and 6th byte of the reply frame
                reply = b'$000' + bytes([
                    HEXCHARS[(level >> 4) & 0xF],
                    HEXCHARS[level & 0xF],
                ]) + b'00'
            elif opcode == OPCODE_SET_LEVEL:
                level = arg
                reply = b'$'
            elif opcode == OPCODE_SET_MODE:
                reply = b'$' if arg <= 2 else b'&'
            else:
                reply = b'$'

            log.write('request %r -> reply %r\n' % (frame, reply))
            os.write(master, reply)


if __name__ == '__main__':
    main()
