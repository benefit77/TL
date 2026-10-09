//-----------------------------------------------------------------------------
//     Author : Seavo Software Research Department
//       Mail : bcc@seavo.com
//        Web : https://www.seavo.com/
//    License : The modified BSD license
//
//    Copyright 2003-2024 seavo.com. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <tchar.h>

// Bus Number, Device Number and Function Number to PCI Device Address
#define PciBusDevFunc(Bus, Dev, Func)	((Bus&0xFF)<<8) | ((Dev&0x1F)<<3) | (Func&7)

//-----------------------------------------------------------------------------
//
// Base Type Defines
//
//-----------------------------------------------------------------------------
#define API_MAX_STR 100
typedef struct _BOARD_BIOS_INFO
{
	CHAR biosVendor[API_MAX_STR];    // Bios Version
	CHAR biosVersion[API_MAX_STR];    // Bios Version
	CHAR biosDate[API_MAX_STR];        // Bios Date
} BOARD_BIOS_INFO;
typedef struct _BOARD_SYSTEM_INFO
{
	CHAR systemManufacturer[API_MAX_STR]; // System manufacturer
	CHAR systemName[API_MAX_STR];         // System name
	CHAR systemVersion[API_MAX_STR];      // System Version
	CHAR systemSerial[API_MAX_STR];       // System Serail Number
	BYTE systemUUID[16];                  // System UUID
} BOARD_SYSTEM_INFO;
typedef struct _BASE_BOARD_INFO
{
	CHAR boardManufacturer[API_MAX_STR]; // Baseboard manufacturer
	CHAR boardName[API_MAX_STR];         // Baseboard name
	CHAR boardVersion[API_MAX_STR];      // Baseboard hardware revision
	CHAR boardSerialNumber[API_MAX_STR]; // Baseboard serial number
} BASE_BOARD_INFO;
typedef struct _CPU_INFO
{
	CHAR cpuName[API_MAX_STR]; // CPU name
	DWORD cpuCount;		// Number of CPUs
	DWORD cpuCoreCount; // Number of cores of each CPU
	DWORD cpuThreadCount; // Number of CPU threads
	//DWORD cpuL1; //L1 Cache
	//DWORD cpuL2; //L2 Cache
	//DWORD cpuL3; //L3 Cache
} CPU_INFO;
typedef struct _MEMORY_INFO
{
	DWORD memTotal; // Total physical memory size in MB
	DWORD memFree; // Free memory in MB
	DWORD memUsage; // Memory usage in percent
	//BYTE  memType[API_MAX_STR]; // Type of memory
} MEMORY_INFO;
typedef struct _PCI_INFO
{
	DWORD bus; // Bus number
	DWORD device; // device number
	DWORD function; // Function number
	WORD deviceId; // Device ID
	WORD vendorId; // Vendor ID
	//BYTE deviceName[API_MAX_STR]; // Name of the device
} PCI_INFO;

//-----------------------------------------------------------------------------
//
// Function Defines
//
//-----------------------------------------------------------------------------
// DLL
typedef VOID (WINAPI *_SvGetDllVersion) (PBYTE major, PBYTE minor, PBYTE revision, PBYTE release);
typedef VOID (WINAPI *_SvGetDriverVersion) (PBYTE major, PBYTE minor, PBYTE revision, PBYTE release);

// INIT
typedef BOOL (WINAPI *_SvApiLibInitialize) ();
typedef VOID (WINAPI *_SvApiLibUnInitialize) ();

// CPU
typedef DWORD (WINAPI *_SvReadMsr) (DWORD index, PDWORD eax, PDWORD edx);
typedef DWORD (WINAPI *_SvWriteMsr) (DWORD index, DWORD eax, DWORD edx);
typedef DWORD (WINAPI *_SvReadPmc) (DWORD index, PDWORD eax, PDWORD edx);
typedef DWORD (WINAPI *_SvCpuid) (DWORD index, PDWORD eax, PDWORD ebx, PDWORD ecx, PDWORD edx);
typedef DWORD (WINAPI *_SvReadTsc) (PDWORD eax, PDWORD edx);

// I/O
typedef BOOL (WINAPI *_SvReadIoPortByteEx) (WORD address, PBYTE value);
typedef BOOL (WINAPI *_SvReadIoPortWordEx) (WORD address, PWORD value);
typedef BOOL (WINAPI *_SvReadIoPortDwordEx) (WORD address, PDWORD value);
typedef BOOL (WINAPI *_SvWriteIoPortByteEx) (WORD address, BYTE value);
typedef BOOL (WINAPI *_SvWriteIoPortWordEx) (WORD address, WORD value);
typedef BOOL (WINAPI *_SvWriteIoPortDwordEx) (WORD address, DWORD value);

// PCI
typedef VOID (WINAPI *_SvGetPciInfo) (BYTE *pPciCount, PCI_INFO *pPciList);
typedef BOOL (WINAPI *_SvReadPciConfigByteEx) (DWORD pciAddress, DWORD regAddress, PBYTE value);
typedef BOOL (WINAPI *_SvReadPciConfigWordEx) (DWORD pciAddress, DWORD regAddress, PWORD value);
typedef BOOL (WINAPI *_SvReadPciConfigDwordEx) (DWORD pciAddress, DWORD regAddress, PDWORD value);
typedef BOOL (WINAPI *_SvWritePciConfigByteEx) (DWORD pciAddress, DWORD regAddress, BYTE value);
typedef BOOL (WINAPI *_SvWritePciConfigWordEx) (DWORD pciAddress, DWORD regAddress, WORD value);
typedef BOOL (WINAPI *_SvWritePciConfigDwordEx) (DWORD pciAddress, DWORD regAddress, DWORD value);

// PhysicalMemory
typedef DWORD (WINAPI *_SvReadPhysicalMemory) (DWORD_PTR address, PBYTE buffer, DWORD count, DWORD unitSize);
typedef DWORD (WINAPI *_SvWritePhysicalMemory) (DWORD_PTR address, PBYTE buffer, DWORD count, DWORD unitSize);

// SMBIOS
typedef BOOL (WINAPI *_SvGetBoardInfo) (BASE_BOARD_INFO *pBoardInfo);
typedef BOOL (WINAPI *_SvGetSystemInfo) (BOARD_SYSTEM_INFO *pSystemInfo);
typedef BOOL (WINAPI *_SvGetBiosInfo) (BOARD_BIOS_INFO *pBiosInfo);
typedef BOOL (WINAPI *_SvGetCPUInfo) (CPU_INFO *pCpuInfo);
typedef BOOL (WINAPI *_SvGetMemoryInfo) (MEMORY_INFO *pMemInfo);

// SMBUS
typedef UINT8 (WINAPI* _SvSmbReadByte) (BYTE addr, BYTE offset, BYTE* data);
typedef UINT8 (WINAPI* _SvSmbReadWord) (BYTE addr, BYTE offset, WORD* data);
typedef UINT8 (WINAPI* _SvSmbReadBlock) (BYTE addr, BYTE offset, BYTE* pData, BYTE* dataLength);
typedef UINT8 (WINAPI* _SvSmbWriteByte) (BYTE addr, BYTE offset, BYTE value);
typedef UINT8 (WINAPI* _SvSmbWriteWord) (BYTE addr, BYTE offset, WORD value);
typedef UINT8 (WINAPI* _SvSmbWriteBlock) (BYTE addr, BYTE offset, BYTE* pData, BYTE dataLength);

// SMBUS transfer
typedef UINT8 (WINAPI* _SvSmbReadByteTransfer) (BYTE addr, BYTE* data);
typedef UINT8 (WINAPI* _SvSmbWriteByteTransfer) (BYTE addr, BYTE command);

// Backlight
typedef BOOL (WINAPI *_SvGetLFPBacklightValue) (BYTE *percentage);
typedef BOOL (WINAPI *_SvSetLFPBacklightValue) (BYTE *percentage);

// HW Monitor
typedef BYTE (WINAPI *_SvGetCPUTemp) (BYTE coreNumber);
typedef BYTE (WINAPI *_SvGetSIOTemp) (BYTE tmpChannel);
typedef WORD (WINAPI *_SvGetVoltageValue) (BYTE adcChannel);
typedef DWORD (WINAPI *_SvGetFanSpeed) (BYTE fanChannel);
typedef BOOL (WINAPI *_SvSetFanPWM) (BYTE fanChannel, BYTE pwm);

// GPIO
typedef DWORD (WINAPI *_SvGetGpioLevel) (DWORD address, DWORD type, DWORD bit);
typedef BOOL (WINAPI *_SvSetGpioLevel) (DWORD address, DWORD type, DWORD bit, DWORD value);
typedef DWORD (WINAPI *_SvGetGpioDirection) (DWORD address, DWORD type, DWORD bitmask);
typedef BOOL (WINAPI *_SvSetGpioDirection) (DWORD address, DWORD type, DWORD bitmask, DWORD direction);

// Watchdog
typedef VOID (WINAPI *_SvWatchdogStop) (VOID);
typedef VOID (WINAPI *_SvWatchdogStart) (BYTE seconds);

// IIC Init
typedef INT (WINAPI* _SvIICInit) (BYTE I2cNumber, DWORD Frequency);
typedef INT (WINAPI* _SvIICUnInit) ();
// IIC 8bit offset
typedef INT (WINAPI* _SvIICReadByte) (WORD addr, BYTE* index, BYTE* data);
typedef INT (WINAPI* _SvIICWriteByte) (WORD addr, BYTE* index, BYTE* data);
typedef INT (WINAPI* _SvIICReadWord) (WORD addr, BYTE* index, WORD* data);
typedef INT (WINAPI* _SvIICWriteWord) (WORD addr, BYTE* index, WORD* data);
typedef INT (WINAPI* _SvIICReadBlock) (WORD addr, BYTE* index, BYTE iCount, BYTE* data);
typedef INT (WINAPI* _SvIICWriteBlock) (WORD addr, BYTE* index, BYTE iCount, BYTE* data);
// IIC 16bit offset
typedef INT (WINAPI* _SvIICReadByteExt) (WORD addr, WORD* index, BYTE* data);
typedef INT (WINAPI* _SvIICWriteByteExt) (WORD addr, WORD* index, BYTE* data);
typedef INT (WINAPI* _SvIICReadWordExt) (WORD addr, WORD* index, WORD* data);
typedef INT (WINAPI* _SvIICWriteWordExt) (WORD addr, WORD* index, WORD* data);
typedef INT (WINAPI* _SvIICReadBlockExt) (WORD addr, WORD* index, BYTE iCount, BYTE* data);
typedef INT (WINAPI* _SvIICWriteBlockExt) (WORD addr, WORD* index, BYTE iCount, BYTE* data);
// IIC Transfer
typedef INT (WINAPI* _SvIICReadByteTransfer) (WORD addr, BYTE* data);
typedef INT (WINAPI* _SvIICReadWordTransfer) (WORD addr, WORD* data);
typedef INT (WINAPI* _SvIICWriteByteTransfer) (WORD addr, BYTE* command);
typedef INT (WINAPI* _SvIICWriteWordTransfer) (WORD addr, WORD* command);

//GetPostMessage
typedef BOOL(WINAPI* _SvGetPostMessage) (BYTE* buffer, BYTE Size);


