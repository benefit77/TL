#include "libledim_proto.h"

uint8_t libledim_Proto_NibbleToAsciiHex (uint8_t arg_nibble) {
	// 0..9 -> '0'..'9', 10..15 -> 'A'..'F'
	if (arg_nibble < 10) {
		return (uint8_t)(arg_nibble + '0');
	}
	if (arg_nibble < 16) {
		return (uint8_t)(arg_nibble - 10 + 'A');
	}

	// out of range, the original implementation answered 0 as well
	return 0;
}

uint8_t libledim_Proto_AsciiHexToNibble (uint8_t arg_ascii_hex) {
	if (arg_ascii_hex >= '0' && arg_ascii_hex <= '9') {
		return (uint8_t)(arg_ascii_hex - '0');
	}
	if (arg_ascii_hex >= 'A' && arg_ascii_hex <= 'F') {
		return (uint8_t)(arg_ascii_hex - 'A' + 10);
	}
	if (arg_ascii_hex >= 'a' && arg_ascii_hex <= 'f') {
		return (uint8_t)(arg_ascii_hex - 'a' + 10);
	}

	// not a hex digit, the original implementation answered 0 as well
	return 0;
}

static uint8_t __calculate_frame_checksum (const uint8_t arg_frame[libledim_Proto_Tx_Frame_Len]) {
	uint8_t tmp_checksum = arg_frame[0];
	tmp_checksum ^= arg_frame[1];
	tmp_checksum ^= arg_frame[2];
	tmp_checksum ^= arg_frame[3];
	tmp_checksum ^= arg_frame[4];
	tmp_checksum ^= arg_frame[5];

	return tmp_checksum;
}

void libledim_Proto_EncodeFrame (
	libledim_Opcode arg_opcode,
	uint8_t arg_channel_order,
	uint8_t arg_op_arg,
	uint8_t arg_out_frame[libledim_Proto_Tx_Frame_Len]
) {
	arg_out_frame[0] = libledim_Proto_Tx_Frame_First_Byte;
	arg_out_frame[1] = (uint8_t)arg_opcode;
	// no masking here: the original encoded the raw channel order, and a half byte is the widest
	// value a single hex digit can carry anyway (the device layer rejects anything above 4 first)
	arg_out_frame[2] = libledim_Proto_NibbleToAsciiHex(arg_channel_order);
	arg_out_frame[3] = '0'; // reserved, always ascii '0'
	arg_out_frame[4] = libledim_Proto_NibbleToAsciiHex((uint8_t)(arg_op_arg >> 4));
	arg_out_frame[5] = libledim_Proto_NibbleToAsciiHex((uint8_t)(arg_op_arg & 0x0F));

	uint8_t tmp_checksum = __calculate_frame_checksum(arg_out_frame);
	arg_out_frame[6] = libledim_Proto_NibbleToAsciiHex((uint8_t)(tmp_checksum >> 4));
	arg_out_frame[7] = libledim_Proto_NibbleToAsciiHex((uint8_t)(tmp_checksum & 0x0F));

	return;
}

uint32_t libledim_Proto_ReplyLenOf (libledim_Opcode arg_opcode) {
	if (arg_opcode == libledim_Opcode_GetChannelLuminanceLevel) {
		// the only command that answers with a payload, the level, instead of a bare retcode
		return libledim_Proto_Rx_Frame_Len;
	}

	return 1;
}

uint8_t libledim_Proto_DecodeLuminanceReply (const uint8_t arg_rx_frame[libledim_Proto_Rx_Frame_Len]) {
	return (uint8_t)(
		(libledim_Proto_AsciiHexToNibble(arg_rx_frame[4]) << 4) |
		libledim_Proto_AsciiHexToNibble(arg_rx_frame[5])
	);
}

bool libledim_Proto_DecodeRetcode (uint8_t arg_rx_byte) {
	return (arg_rx_byte == libledim_Proto_Retcode_Success);
}
