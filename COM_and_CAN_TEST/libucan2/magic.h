#ifndef __libucan2_magic_h
#define __libucan2_magic_h

#include "Comproto.h"

#define libucan2_MG_usb_dev_vid						0x0483
#define libucan2_MG_usb_dev_pid						0x5751

#define libucan2_MG_usb_hid_in_report_buf_size		(Comproto_frame_len+1)
#define libucan2_MG_usb_hid_out_report_buf_size		(Comproto_frame_len+1)
#define libucan2_MG_usb_hid_in_report_tag			0x01
#define libucan2_MG_usb_hid_out_report_tag			0x02
#define libucan2_MG_usb_hid_in_endpoint_number		0x81
#define libucan2_MG_usb_hid_out_endpoint_number		0x01

#define libucan2_MG_min_usb_interrupt_transfer_timeout		50 // ms
#define libucan2_MG_default_usb_interrupt_transfer_timeout	2000 // ms

#define libucan2_MG_ping_random_byteseq_len			12

#define libucan2_MG_max_can_chan_order				2
#define libucan2_MG_min_nominal_baud_rate			40000 // 40kbps
#define libucan2_MG_max_nominal_baud_rate			1000000 // 1Mbps
#define libucan2_MG_min_data_baud_rate				1000000 // 1Mbps
#define libucan2_MG_max_data_baud_rate				6000000 // 6Mbps

#define libucan2_MG_rest_tx_frame_data_buf_size		32 // for max 64 bytes can frame data, the first 32 bytes are sent first, then follow the rest 32 bytes

#endif // __libucan2_magic_h
