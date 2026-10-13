#!/usr/bin/env python3
"""Drive the library without hardware, and compare it against the library it replaces.

Two modes:

  integration   runs the command line front end against a stand-in controller (a pty whose other
                end answers the frame protocol) and checks what it reports, including the cases
                that must fail: a controller that refuses a command, a port that answers nothing,
                a path that is not a serial port at all

  differential  compiles one trace program twice, once against the first generation of libledim and
                once against this one, runs both against their own stand-in controller, and demands
                that the two agree on both the values they returned and the bytes they sent

the differential mode is the one that matters: it is what turns "the protocol looks the same" into
"the protocol is the same".
"""

import argparse
import difflib
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

_fstatic_num_checks = 0
_fstatic_failures = []


def check(condition, description, detail=None):
    global _fstatic_num_checks
    _fstatic_num_checks += 1
    if condition:
        return True
    message = 'FAIL %s' % description
    if detail:
        message += '\n%s' % detail
    _fstatic_failures.append(message)
    print(message)
    return False


def check_contains(haystack, needle, description):
    return check(needle in haystack, description,
                 'expected to find %r in:\n%s' % (needle, haystack))


class FakeController:
    """The stand-in controller, on the far end of a pty pair."""

    def __init__(self, script, work_dir, name, silent=False, reject_channel=4, level=0xAB):
        self.port_file = os.path.join(work_dir, '%s.port' % name)
        self.log_file = os.path.join(work_dir, '%s.log' % name)
        for path in (self.port_file, self.log_file):
            if os.path.exists(path):
                os.remove(path)

        command = [
            sys.executable, script,
            '--port-file', self.port_file,
            '--log-file', self.log_file,
            '--reject-channel', str(reject_channel),
            '--level', hex(level),
        ]
        if silent:
            command.append('--silent')

        self.process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.portname = self._wait_for_portname()

    def _wait_for_portname(self):
        deadline = time.time() + 10
        while time.time() < deadline:
            if os.path.exists(self.port_file):
                with open(self.port_file) as port_file:
                    portname = port_file.read().strip()
                if portname:
                    return portname
            if self.process.poll() is not None:
                raise RuntimeError('the stand-in controller died: %s' % self.process.stdout.read().decode())
            time.sleep(0.01)
        raise RuntimeError('the stand-in controller did not come up')

    def requests(self):
        """The request frames the controller saw, in order."""
        with open(self.log_file) as log_file:
            lines = log_file.readlines()
        return [line.strip() for line in lines if line.startswith('request ')]

    def close(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()


def run_cli(cli, arguments, timeout=30):
    completed = subprocess.run([cli] + arguments, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               timeout=timeout)
    return completed.returncode, completed.stdout.decode(errors='replace')


def mode_integration(args):
    controller = FakeController(args.fake, args.work_dir, 'integration')
    generous = FakeController(args.fake, args.work_dir, 'integration_noreject', reject_channel=0)
    silent = FakeController(args.fake, args.work_dir, 'integration_silent', silent=True)
    try:
        # the port listing does not need a controller at all
        retcode, output = run_cli(args.cli, ['list'])
        check(retcode in (0, 1), 'list exits 0 when it found ports, 1 when it found none', output)
        check_contains(output, 'serial port(s) found', 'list reports what it found')
        for portname in re.findall(r'\[\d+\] (\S+)', output):
            check(portname.startswith('/dev/'), 'list only reports device paths: %s' % portname)

        # turning a channel on and off
        retcode, output = run_cli(args.cli, ['-p', controller.portname, 'on', '1'])
        check(retcode == 0, 'turning channel 1 on succeeds', output)
        check_contains(output, 'turn on: channel 1, ok', 'the successful turn on is reported')
        check_contains(output, 'libledim bound to %s' % controller.portname, 'the bound port is reported')

        retcode, output = run_cli(args.cli, ['-p', controller.portname, 'off', '2'])
        check(retcode == 0, 'turning channel 2 off succeeds', output)
        check_contains(output, 'turn off: channel 2, ok', 'the successful turn off is reported')

        # the luminance level, set and read back, and the frame that carries it
        retcode, output = run_cli(args.cli, ['-p', controller.portname, 'level', '3', '200'])
        check(retcode == 0, 'setting a luminance level succeeds', output)
        check_contains(output, 'set level: channel 3, level 200, ok (read back 200)',
                       'the level is set and read back')
        check(
            any("b'$330C86F'" in request for request in controller.requests()),
            'the controller received the frame of a level 200 on channel 3',
            'requests: %s' % controller.requests()
        )

        retcode, output = run_cli(args.cli, ['-p', controller.portname, 'get', '3'])
        check(retcode == 0, 'reading a luminance level back succeeds', output)
        check_contains(output, 'channel 3: luminance level 200', 'the level that was set is read back')

        # the modes and the flash
        retcode, output = run_cli(args.cli, ['-p', controller.portname, 'mode', '2', '2'])
        check(retcode == 0, 'setting the flash mode succeeds', output)

        retcode, output = run_cli(args.cli, ['-p', controller.portname, 'flash', '2', '120'])
        check(retcode == 0, 'flashing a channel succeeds', output)
        check_contains(output, 'flash: channel 2, 120 ms, ok', 'the flash is reported')

        # a controller that refuses a command: the reason has to reach the operator
        retcode, output = run_cli(args.cli, ['-p', controller.portname, 'on', '4'])
        check(retcode == 1, 'a refused command exits non zero', output)
        check_contains(output, 'turn on: channel 4, FAILED', 'a refused command is reported as a failure')
        check_contains(output, 'refused', 'the reason of a refused command is reported')

        # an argument the tool itself refuses, before the library is ever involved
        retcode, output = run_cli(args.cli, ['-p', controller.portname, 'mode', '1', '3'])
        check(retcode == 2, 'an invalid mode is refused as a usage error', output)
        check_contains(output, 'error: mode needs a channel', 'the usage error explains itself')
        # a set mode on channel 1 would be the frame "$81...", and it must never have been sent
        check(not any("b'$81" in request for request in controller.requests()),
              'the refused mode never reached the wire',
              'requests: %s' % controller.requests())

        # a port that exists but answers nothing
        retcode, output = run_cli(args.cli, ['-t', '200', '-p', silent.portname, 'on', '1'])
        check(retcode == 1, 'a port that answers nothing exits non zero', output)
        check_contains(output, 'did not answer in time', 'a silent port is reported as a timeout')

        # a path that is not a serial port
        retcode, output = run_cli(args.cli, ['-p', '/dev/definitely-not-a-port', 'on', '1'])
        check(retcode == 1, 'a path that is not a serial port exits non zero', output)
        check_contains(output, 'could not be opened', 'a bad path is reported as an open failure')

        # the whole sweep, on a controller that refuses nothing
        retcode, output = run_cli(args.cli, ['-p', generous.portname, 'selftest'])
        check(retcode == 0, 'the selftest sweep succeeds on a healthy controller', output[-2000:])
        check_contains(output, 'selftest: all transactions succeeded', 'the selftest reports its verdict')

        # the whole sweep, on a controller that refuses one channel
        retcode, output = run_cli(args.cli, ['-p', controller.portname, 'selftest'])
        check(retcode == 1, 'the selftest sweep fails when a channel is refused', output[-2000:])
        check_contains(output, 'transaction(s) failed', 'the selftest counts the failures')
    finally:
        controller.close()
        generous.close()
        silent.close()


def mode_gui(args):
	"""Start the window without a screen, let it click through one channel, and read the wire."""
	controller = FakeController(args.fake, args.work_dir, 'gui')
	try:
		environment = dict(os.environ)
		# the window is driven without a display, which is also how it is checked on a build machine
		environment['QT_QPA_PLATFORM'] = 'offscreen'
		completed = subprocess.run(
			[args.gui, '-p', controller.portname, '--selfcheck'],
			env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
		output = completed.stdout.decode(errors='replace')
		check(completed.returncode == 0, 'the window starts and exits cleanly', output)

		frames = controller.requests()
		# the handshake of the connect, then the five commands the self check presses through
		for description, frame in [
				('the connect handshake', "b'$4100011'"),
				('the level the slider asked for', "b'$3107819'"),
				('the turn on of the 开 button', "b'$1100014'"),
				('the read back of the Read button', "b'$4100011'"),
				('the flash of the 触发 button', "b'$7100012'"),
				('the turn off of the 关 button', "b'$2100017'")]:
			check(any(frame in request for request in frames),
				'the window sent %s' % description,
				'frames: %s' % frames)
	finally:
		controller.close()


def compile_trace(source, include_dir, lib_dir, output):
    command = [
        'cc', '-std=c99', '-Wall', '-Wextra',
        '-I', include_dir,
        source,
        '-L', lib_dir, '-lledim',
        '-Wl,-rpath,%s' % lib_dir,
        '-o', output,
    ]
    completed = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if completed.returncode != 0:
        raise RuntimeError('could not compile the trace against %s:\n%s'
                           % (lib_dir, completed.stdout.decode(errors='replace')))


def run_trace(trace_binary, name, args):
    """Run the trace against its own stand-in controller, and collect both transcripts."""
    controller = FakeController(args.fake, args.work_dir, name)
    silent = FakeController(args.fake, args.work_dir, '%s_silent' % name, silent=True)
    try:
        completed = subprocess.run([trace_binary, controller.portname, silent.portname],
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
        transcript = completed.stdout.decode(errors='replace')
        # the pty of this run only exists in this run, so it cannot take part in the comparison
        transcript = transcript.replace(controller.portname, '<port>')
        frames = [request.replace(controller.portname, '<port>') for request in controller.requests()]
        return completed.returncode, transcript, frames
    finally:
        controller.close()
        silent.close()


def mode_differential(args):
    new_include_dir = os.path.join(args.source_dir, 'libledim', 'include')
    trace_source = os.path.join(args.source_dir, '测试', 'differential_test.c')

    new_trace = os.path.join(args.work_dir, 'trace_new')
    old_trace = os.path.join(args.work_dir, 'trace_old')
    compile_trace(trace_source, new_include_dir, args.lib_dir, new_trace)
    # the first generation kept its header one directory deeper, under <inc>/libledim/
    compile_trace(trace_source, args.baseline_include_dir, args.baseline_lib_dir, old_trace)

    new_retcode, new_transcript, new_frames = run_trace(new_trace, 'diff_new', args)
    old_retcode, old_transcript, old_frames = run_trace(old_trace, 'diff_old', args)

    check(new_retcode == old_retcode, 'both libraries exit with the same status')
    if new_transcript != old_transcript:
        diff = '\n'.join(difflib.unified_diff(
            old_transcript.splitlines(), new_transcript.splitlines(),
            fromfile='first generation', tofile='rewritten', lineterm=''))
        check(False, 'both libraries answer the same values', diff)
    else:
        check(True, 'both libraries answer the same values')

    if new_frames != old_frames:
        diff = '\n'.join(difflib.unified_diff(
            old_frames, new_frames,
            fromfile='first generation', tofile='rewritten', lineterm=''))
        check(False, 'both libraries put the same bytes on the wire', diff)
    else:
        check(True, 'both libraries put the same bytes on the wire (%d frames)' % len(new_frames))


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--cli', help='the libledim-clidemo built from this source tree')
    parser.add_argument('--gui', help='the libledim-gui built from this source tree')
    parser.add_argument('--lib-dir', help='the directory holding the libledim built from this tree')
    parser.add_argument('--baseline-lib-dir', help='the directory holding a first generation libledim')
    parser.add_argument('--baseline-include-dir', help='the include directory of a first generation libledim')
    parser.add_argument('--test-dir', default=None, help='where the pty names and the logs land')
    parser.add_argument('--mode', choices=['integration', 'gui', 'differential', 'all'], default='all')
    args = parser.parse_args()

    args.source_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    args.fake = os.path.join(args.source_dir, '测试', 'fake_led.py')

    work_dir = args.test_dir or tempfile.mkdtemp(prefix='libledim-tests-')
    os.makedirs(work_dir, exist_ok=True)
    args.work_dir = work_dir

    if args.mode in ('integration', 'all'):
        if not args.cli:
            parser.error('--cli is needed for the integration mode')
        mode_integration(args)

    if args.mode in ('gui', 'all'):
        if not args.gui:
            if args.mode == 'gui':
                parser.error('--gui is needed for the gui mode')
            print('gui mode skipped, no window binary was given')
        elif not os.path.exists(args.gui):
            print('gui mode skipped, %s does not exist' % args.gui)
        else:
            mode_gui(args)

    if args.mode in ('differential', 'all'):
        if not (args.baseline_lib_dir and args.baseline_include_dir):
            if args.mode == 'differential':
                parser.error('--baseline-lib-dir and --baseline-include-dir are needed for the '
                             'differential mode')
            print('differential mode skipped, no first generation library to compare against')
        elif not args.lib_dir:
            parser.error('--lib-dir is needed for the differential mode')
        else:
            mode_differential(args)

    print()
    if _fstatic_failures:
        print('%d check(s), %d failure(s)' % (_fstatic_num_checks, len(_fstatic_failures)))
        return 1

    print('%d check(s), 0 failure(s)' % _fstatic_num_checks)
    return 0


if __name__ == '__main__':
    sys.exit(main())
