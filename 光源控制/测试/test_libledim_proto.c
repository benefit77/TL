/*
* the protocol, checked against fixed vectors
*
* the vectors are not round tripped through the implementation, they are written out by hand from
* the protocol description, which is the entire point: an encoder that agrees with a decoder it was
* written next to proves nothing about the wire. the one that settles it is the differential test,
* which compares these bytes against what the first generation of libledim actually put on the wire.
*/

#include "libledim_proto.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static int _fstatic_num_checks = 0;
static int _fstatic_num_failures = 0;

static void __report_frame (const uint8_t* arg_expected, const uint8_t* arg_actual, const char* arg_case) {
	_fstatic_num_checks++;
	if (memcmp(arg_expected, arg_actual, libledim_Proto_Tx_Frame_Len) == 0) {
		return;
	}

	_fstatic_num_failures++;
	printf("FAIL %s\n", arg_case);
	printf("  expected: ");
	for (int tmp_i = 0; tmp_i < libledim_Proto_Tx_Frame_Len; tmp_i++) {
		printf("%02X ", arg_expected[tmp_i]);
	}
	printf("\n  actual:   ");
	for (int tmp_i = 0; tmp_i < libledim_Proto_Tx_Frame_Len; tmp_i++) {
		printf("%02X ", arg_actual[tmp_i]);
	}
	printf("\n");

	return;
}

static void __check_frame (const char* arg_case, libledim_Opcode arg_opcode, uint8_t arg_channel, uint8_t arg_op_arg, const char* arg_expected_ascii) {
	uint8_t tmp_frame[libledim_Proto_Tx_Frame_Len] = { 0 };
	libledim_Proto_EncodeFrame(arg_opcode, arg_channel, arg_op_arg, tmp_frame);
	__report_frame((const uint8_t*)arg_expected_ascii, tmp_frame, arg_case);

	return;
}

static void __check_uint (const char* arg_case, uint32_t arg_expected, uint32_t arg_actual) {
	_fstatic_num_checks++;
	if (arg_expected == arg_actual) {
		return;
	}

	_fstatic_num_failures++;
	printf("FAIL %s: expected %u, got %u\n", arg_case, arg_expected, arg_actual);

	return;
}

static void __check_bool (const char* arg_case, bool arg_expected, bool arg_actual) {
	_fstatic_num_checks++;
	if (arg_expected == arg_actual) {
		return;
	}

	_fstatic_num_failures++;
	printf("FAIL %s: expected %s, got %s\n", arg_case, arg_expected ? "true" : "false", arg_actual ? "true" : "false");

	return;
}

static void test_encoder (void) {
	// the frame layout is '$', opcode, channel, '0', arg high nibble, arg low nibble, checksum
	// high nibble, checksum low nibble, where the checksum is the xor of the first six bytes
	__check_frame("get luminance, channel 1", libledim_Opcode_GetChannelLuminanceLevel, 1, 0, "$4100011");
	__check_frame("turn channel 1 on", libledim_Opcode_TurnOnChannel, 1, 0, "$1100014");
	__check_frame("turn channel 4 off", libledim_Opcode_TurnOffChannel, 4, 0, "$2400012");
	__check_frame("set luminance, channel 3 to 200", libledim_Opcode_SetChannelLuminanceLevel, 3, 200, "$330C86F");
	__check_frame("set luminance, channel 1 to 71", libledim_Opcode_SetChannelLuminanceLevel, 1, 71, "$3104715");
	__check_frame("set luminance, channel 2 to 0", libledim_Opcode_SetChannelLuminanceLevel, 2, 0, "$3200015");
	__check_frame("set mode 2, channel 2", libledim_Opcode_SetChannelMode, 2, 2, "$820021C");
	__check_frame("trigger flash, channel 2", libledim_Opcode_TriggerChannelFlash, 2, 0, "$7200011");
	__check_frame("set flash duration 250ms, channel 4", libledim_Opcode_SetChannelFlashDuration, 4, 250, "$940FA1E");

	return;
}

static void test_decoder (void) {
	uint8_t tmp_rx[libledim_Proto_Rx_Frame_Len] = { 0 };

	// the luminance rides on the 5th and 6th byte, as two ascii hex digits
	memcpy(tmp_rx, "$000AB00", libledim_Proto_Rx_Frame_Len);
	__check_uint("decode luminance 0xAB", 0xAB, libledim_Proto_DecodeLuminanceReply(tmp_rx));

	memcpy(tmp_rx, "$000ab00", libledim_Proto_Rx_Frame_Len);
	__check_uint("decode luminance 0xAB, lowercase", 0xAB, libledim_Proto_DecodeLuminanceReply(tmp_rx));

	memcpy(tmp_rx, "$0000000", libledim_Proto_Rx_Frame_Len);
	__check_uint("decode luminance 0", 0, libledim_Proto_DecodeLuminanceReply(tmp_rx));

	memcpy(tmp_rx, "$000FF00", libledim_Proto_Rx_Frame_Len);
	__check_uint("decode luminance 255", 255, libledim_Proto_DecodeLuminanceReply(tmp_rx));

	// a byte that is not a hex digit reads as 0, which is what the firmware era implementation did
	memcpy(tmp_rx, "$000zz00", libledim_Proto_Rx_Frame_Len);
	__check_uint("decode luminance, garbage nibbles", 0, libledim_Proto_DecodeLuminanceReply(tmp_rx));

	__check_bool("retcode '$' is accepted", true, libledim_Proto_DecodeRetcode('$'));
	__check_bool("retcode '&' is refused", false, libledim_Proto_DecodeRetcode('&'));
	__check_bool("retcode 0 is refused", false, libledim_Proto_DecodeRetcode(0));
	__check_bool("retcode 'x' is refused", false, libledim_Proto_DecodeRetcode('x'));

	// only the luminance read back answers with a payload, every other command answers with one byte
	__check_uint("reply length of the luminance read back", 8, libledim_Proto_ReplyLenOf(libledim_Opcode_GetChannelLuminanceLevel));
	__check_uint("reply length of turn on", 1, libledim_Proto_ReplyLenOf(libledim_Opcode_TurnOnChannel));
	__check_uint("reply length of turn off", 1, libledim_Proto_ReplyLenOf(libledim_Opcode_TurnOffChannel));
	__check_uint("reply length of set luminance", 1, libledim_Proto_ReplyLenOf(libledim_Opcode_SetChannelLuminanceLevel));
	__check_uint("reply length of trigger flash", 1, libledim_Proto_ReplyLenOf(libledim_Opcode_TriggerChannelFlash));
	__check_uint("reply length of set mode", 1, libledim_Proto_ReplyLenOf(libledim_Opcode_SetChannelMode));
	__check_uint("reply length of set flash duration", 1, libledim_Proto_ReplyLenOf(libledim_Opcode_SetChannelFlashDuration));

	return;
}

static void test_nibble_conversion (void) {
	__check_uint("nibble 0", '0', libledim_Proto_NibbleToAsciiHex(0));
	__check_uint("nibble 9", '9', libledim_Proto_NibbleToAsciiHex(9));
	__check_uint("nibble 10", 'A', libledim_Proto_NibbleToAsciiHex(10));
	__check_uint("nibble 15", 'F', libledim_Proto_NibbleToAsciiHex(15));
	__check_uint("nibble 16 is out of range", 0, libledim_Proto_NibbleToAsciiHex(16));

	__check_uint("ascii '0'", 0, libledim_Proto_AsciiHexToNibble('0'));
	__check_uint("ascii '9'", 9, libledim_Proto_AsciiHexToNibble('9'));
	__check_uint("ascii 'A'", 10, libledim_Proto_AsciiHexToNibble('A'));
	__check_uint("ascii 'f'", 15, libledim_Proto_AsciiHexToNibble('f'));
	__check_uint("ascii 'G' is not a digit", 0, libledim_Proto_AsciiHexToNibble('G'));

	// round trip over the whole nibble range
	for (uint8_t tmp_nibble = 0; tmp_nibble < 16; tmp_nibble++) {
		__check_uint(
			"nibble round trip",
			tmp_nibble,
			libledim_Proto_AsciiHexToNibble(libledim_Proto_NibbleToAsciiHex(tmp_nibble))
		);
	}

	return;
}

int main (void) {
	test_encoder();
	test_decoder();
	test_nibble_conversion();

	printf("%d checks, %d failure(s)\n", _fstatic_num_checks, _fstatic_num_failures);

	return (_fstatic_num_failures == 0) ? 0 : 1;
}
