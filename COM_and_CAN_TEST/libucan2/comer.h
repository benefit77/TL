#ifndef __libucan2_comer_h
#define __libucan2_comer_h

#include <YAC/Compat/libc/stdbool.h>
#include <YAC/Compat/libc/stdint.h>
#include <YAC/LWidget/U8Serdes.h>

#include "Comproto.h"

extern U8Serdes* comer_prepare_to_send_frame (Comproto_Opcode arg_opcode);
extern bool comer_send_frame ();
extern U8Serdes* comer_recv_frame(Comproto_Opcode arg_expected_rx_opcode);
extern bool comer_ping ();
extern u32 comer_read_fw_ver ();

#endif // __libucan2_comer_h
