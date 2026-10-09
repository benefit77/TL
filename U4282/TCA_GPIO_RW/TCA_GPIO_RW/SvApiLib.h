//-----------------------------------------------------------------------------
//     Author : Seavo Software Research Department
//       Mail : bcc@seavo.com
//        Web : https://www.seavo.com/
//    License : The modified BSD license
//
//    Copyright 2003-2022 seavo.com. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef _SV_API_LIB_H_
#define _SV_API_LIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <stdbool.h>
#include <dirent.h>
#include <poll.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/input.h>
#include <linux/hidraw.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/io.h>
#include <dirent.h>
#include <inttypes.h>
#include <getopt.h>
#include <cpuid.h>

typedef unsigned char         UINT8;
typedef unsigned short        UINT16;
typedef unsigned int          UINT32;
typedef unsigned long long    UINT64;

typedef struct cpuid {
	UINT32 eax, ebx, ecx, edx;
} _cpuid;

#define PCI_IO_INDEX  0xCF8
#define PCI_IO_DATA   0xCFC
#define CMOS_IO_INDEX 0x70
#define CMOS_IO_DATA  0x71
#define IO_INDEX_2E   0x2E
#define IO_INDEX_2F   0x2F
#define IO_INDEX_4E   0x4E
#define IO_INDEX_4F   0x4F

#define delayms(x) usleep(x*1000)
#define delayus(x) usleep(x)

#define OEM_PCI_ADDRESS(Bus, Device, Function, Register) \
( (1<<31) | (((Bus) & 0xFF) << 16) | (((Device) & 0x1F) << 11) | (((Function) & 0x07) << 8) | ((Register) & 0xFF) )

#define BIT(x) (1ULL << (x))
#define MIN(x,y) ( ((x) > (y)) ? (y) : (x) )
#define MAX(x,y) ( ((x) > (y)) ? (x) : (y) )

#ifdef __cplusplus
extern "C" {
#endif

   bool   SvApiLibInit(void);
   void   SvApiLibUnInit(void);

   UINT8  SvIoRead8 (UINT16 addr);
   UINT16 SvIoRead16(UINT16 addr);
   UINT32 SvIoRead32(UINT16 addr);
   void   SvIoWrite8 (UINT16 addr, UINT8  data);
   void   SvIoWrite16(UINT16 addr, UINT16 data);
   void   SvIoWrite32(UINT16 addr, UINT32 data);

   UINT32 SvMmioRead32 (UINT32 addr);
   void   SvMmioWrite32(UINT32 addr, UINT32 val);
   
   UINT8  SvSmbReadByte(UINT8 DEVADD, UINT8 index);
   void   SvSmbWriteByte(UINT8 DEVADD, UINT8 index, UINT8 data);

   // pthread
   void   SvReadMsr (UINT32 reg, int cpu, UINT64 *val);
   void   SvDumpMsrAll(UINT32 reg);
   void   SvWriteMsr(UINT32 reg, int cpu, UINT64 data);
   void   SvWriteMsrAll(UINT32 reg,  UINT64 data);
   int    SvCpuid(int cpu, UINT32 leaf, UINT32 subleaf, struct cpuid *data);

   // asm
   //void   SvWriteMsr64 (UINT32 reg, UINT64 Value);
   //UINT64 SvReadMsr64  (UINT32 reg);
   int    SvGetCpuid(int leaf, UINT32 *eax, UINT32 *ebx, UINT32 *ecx, UINT32 *edx);

   UINT32 SvPciConfigRead32(UINT32 bus, UINT32 device, UINT32 function, UINT32 offset);
   void   SvPciConfigWrite32(UINT32 bus, UINT32 device, UINT32 function, UINT32 offset, UINT32 value);

#ifdef __cplusplus
}
#endif 

#endif






