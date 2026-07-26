#ifndef __UGame_Comproto_h
#define __UGame_Comproto_h

// the longest frame (send/recv can frame): 1 opcode + 1 bitfield + 1 frame_sernum + 1 data_len + 4 msg_id + 32 msg_data = 40 bytes
#define Comproto_frame_len 40

enum Comproto_Opcode {
	Comproto_Opcode_ping,
	// ret=u32 (msb is always 0, follows 3 u8 in the order of fw_ver major, minor, patch)
	Comproto_Opcode_read_firmware_version,
	
	/*
	host to mcu:
		* u8 -> opcode
		* u8 -> chan_index
	mcu to host:
		* u8 -> opcode
		* u8 -> bitfield:
			bit.0 -> ret_bool
			bit.1 -> chan_index
			bit.2 -> chan_endis
			bit.3 -> is_accept_all
			bit.4 -> std_filter_endis
			bit.5 -> ext_filter_endis
		* u8 -> num_rx_frames
		* u32 -> nominal_baud_rate
		* u32 -> data_baud_rate
		* u32 -> std_filter_id
		* u32 -> std_filter_mask
		* u32 -> ext_filter_id
		* u32 -> ext_filter_mask
	*/
	Comproto_Opcode_read_channnel_status,
	
	/*
	host to mcu:
		* u8 -> opcode
		* u8 -> bitfield
			bit.0 -> chan_index
			bit.1 -> is_accept_all
		* u32 -> nominal_baud_rate
		* u32 -> data_baud_rate
	mcu to host:
		* u8 -> opcode
		* u8 -> ret_bool
	*/
	Comproto_Opcode_init_channel,
	
	/*
	host to mcu:
		* u8 -> opcode
		* u8 -> bitfield
			bit.0 -> chan_index
			bit.1 -> endis
	mcu to host:
		* u8 -> opcode
		* u8 -> ret_bool
	*/
	Comproto_Opcode_endis_channel,
	
	/*
	host to mcu:
		* u8 -> opcode
		* u8 -> bitfield
			bit.0 -> chan_index
			bit.1 -> std_or_ext
		* u32 -> filter_id
		* u32 -> filter_mask
	mcu to host:
		* u8 -> opcode
		* u8 -> ret_bool
	*/
	Comproto_Opcode_set_filter,
	
	/*
	host to mcu:
		* u8 -> opcode
		* u8 -> bitfield
			bit.0 -> chan_index
			bit.1 -> endis
			bit.2 -> std_or_ext
	mcu to host:
		* u8 -> opcode
		* u8 -> ret_bool
	*/
	Comproto_Opcode_endis_filter,
	
	/*
	host to mcu:
		* u8 -> opcode
		* u8 -> bitfield
			bit.0 -> chan_index
		* u32 -> timeout_ms
	mcu to host:
		* u8 -> opcode
		* u8 -> ret_bool
	*/
	Comproto_Opcode_set_tx_timeout,
	/*
	host to mcu:
		* u8 -> opcode
		* u8 -> bitfield
			bit.0 -> chan_index
			bit.1 -> is_std_msg_id
			bit.2 -> is_classic_frame
			bit.3 -> is_data_frame
			bit.4 -> use_brs
		* u8 -> frame_sernum
		* u8 -> data_len
		* u32 -> msg_id
		* up to 32 bytes frame_data (for 48 or 64 bytes data, send the rest with Comproto_Opcode_frame_data with same frame_serial_num)
	mcu to host:
		* u8 -> opcode continue_frame_data
	mcu to host:
		* u8 -> opcode send_frame
		* u8 -> ret_bool
	*/
	Comproto_Opcode_send_frame,
	/*
	host to mcu:
		* u8 -> opcode
		* u8 -> chan_index
	mcu to host:
		* u8 -> opcode
		* u8 -> bitfield
			bit.0 -> ret_bool
			bit.1 -> chan_index
			bit.2 -> is_std_msg_id
			bit.3 -> is_classic_frame
			bit.4 -> is_data_frame
			bit.5 -> use_brs
		* u8 -> frame_sernum
		* u8 -> data_len
		* u32 -> msg_id
		* up to 32 bytes data (for 48 or 64 bytes data, send the rest with Comproto_Opcode_frame_data with same frame_serial_num)
	*/
	Comproto_Opcode_recv_frame,
	
	Comproto_Opcode_endis_frame_listening,
	Comproto_Opcode_frame_received,
	/*
	host to mcu or mcu to host:
		* u8 -> opcode
		* u8 -> bitfield
			bit.0 -> ret_bool
			bit.1 -> chan_index
		* u8 -> frame_sernum
	*/
	Comproto_Opcode_continue_frame_data,
	/*
	host to mcu (sent out rest tx frame data) / mcu to host (sent out rest rx frame data) upon recving continue_frame_data:
		* u8 -> opcode
		* u8 -> bitfield
			bit.0 -> ret_bool
			bit.1 -> chan_index
		* u8 -> frame_sernum
		* u8 -> rest_frame_data_len
		* up to 32 bytes data
	mcu to host (host does not respond to this opcode):
		* u8 -> opcode send_frame
		* u8 -> ret_bool
	*/
	Comproto_Opcode_rest_frame_data
};

#ifndef __cplusplus
	typedef enum Comproto_Opcode Comproto_Opcode;
#endif

#endif // __UGame_Comproto_h
