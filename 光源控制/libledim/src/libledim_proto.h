#ifndef __libledim_proto_h
#define __libledim_proto_h

/*
* the wire protocol of the light source controller, and nothing else
*
* every function here is pure: no port, no state, no allocation. that is deliberate, it is what
* makes the protocol testable (and the test vectors greppable) without any hardware around
*
* a transaction is one 8 byte request frame, followed by a reply:
*   - 8 bytes, for the luminance read back
*   - 1 byte,  for every other command ('$' accepted, '&' refused)
*/

#include <stdbool.h>
#include <stdint.h>

#define libledim_Proto_Tx_Frame_Len 8
#define libledim_Proto_Rx_Frame_Len 8

// the first byte of every request frame, and the byte a controller answers with on success
#define libledim_Proto_Tx_Frame_First_Byte 0x24 // ascii '$'
#define libledim_Proto_Retcode_Success 0x24 // ascii '$'
#define libledim_Proto_Retcode_Failed 0x26 // ascii '&'

/*
* the opcode is the second byte of the frame
* the values are ascii digits, which is what makes a captured trace readable
*/
typedef enum libledim_Opcode {
	libledim_Opcode_TurnOnChannel = 0x31, // ascii '1'
	libledim_Opcode_TurnOffChannel = 0x32, // ascii '2'
	libledim_Opcode_SetChannelLuminanceLevel = 0x33, // ascii '3'
	libledim_Opcode_GetChannelLuminanceLevel = 0x34, // ascii '4'
	libledim_Opcode_TriggerChannelFlash = 0x37, // ascii '7'
	libledim_Opcode_SetChannelMode = 0x38, // ascii '8'
	libledim_Opcode_SetChannelFlashDuration = 0x39 // ascii '9'
} libledim_Opcode;

/*
* build the 8 byte request frame:
*   [0] '$'
*   [1] opcode
*   [2] channel order, as an ascii hex digit
*   [3] '0' (reserved, the firmware expects this to be '0')
*   [4] the high nibble of arg, as an ascii hex digit
*   [5] the low nibble of arg, as an ascii hex digit
*   [6] the high nibble of the checksum, as an ascii hex digit
*   [7] the low nibble of the checksum, as an ascii hex digit
* the checksum is the xor of the first 6 bytes
*/
void libledim_Proto_EncodeFrame (
	libledim_Opcode arg_opcode,
	uint8_t arg_channel_order,
	uint8_t arg_op_arg,
	uint8_t arg_out_frame[libledim_Proto_Tx_Frame_Len]
);

// how many reply bytes a command is answered with (8 for the luminance read back, 1 otherwise)
uint32_t libledim_Proto_ReplyLenOf (libledim_Opcode arg_opcode);

// the luminance level carried by a reply frame, off its [4] and [5] bytes
uint8_t libledim_Proto_DecodeLuminanceReply (const uint8_t arg_rx_frame[libledim_Proto_Rx_Frame_Len]);

// whether the single reply byte says "accepted"
bool libledim_Proto_DecodeRetcode (uint8_t arg_rx_byte);

// ascii hex digit <-> 4 bit integer, exposed for the tests
uint8_t libledim_Proto_NibbleToAsciiHex (uint8_t arg_nibble);
uint8_t libledim_Proto_AsciiHexToNibble (uint8_t arg_ascii_hex);

#endif // __libledim_proto_h
