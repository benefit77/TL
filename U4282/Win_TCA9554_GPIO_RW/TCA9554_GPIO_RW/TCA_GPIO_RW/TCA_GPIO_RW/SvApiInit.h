//-----------------------------------------------------------------------------
//     Author : Seavo Software Research Department
//       Mail : bcc@seavo.com
//        Web : https://www.seavo.com/
//    License : The modified BSD license
//
//    Copyright 2003-2024 seavo.com. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include "SvApiInitDef.h"

//-----------------------------------------------------------------------------
//
// Prototypes
//
//-----------------------------------------------------------------------------

BOOL InitApiLib(HMODULE *hModule);
BOOL DeinitApiLib(HMODULE *hModule);

BYTE IoRead8(WORD port);
WORD IoRead16(WORD port);
DWORD IoRead32(WORD port);
BOOL IoWrite8(WORD port, BYTE value);
BOOL IoWrite16(WORD port, WORD value);
BOOL IoWrite32(WORD port, DWORD value);

DWORD MmioRead32(DWORD addr);
VOID MmioWrite32(DWORD addr, DWORD value);

DWORD PciRead32(BYTE bus, BYTE device, BYTE function, DWORD offset);
VOID PciWrite32(BYTE bus, BYTE device, BYTE function, DWORD offset, DWORD value);

//-----------------------------------------------------------------------------
//
// Funtions
//
//-----------------------------------------------------------------------------
// DLL
_SvGetDllVersion GetDllVersion = NULL;
_SvGetDriverVersion GetDriverVersion = NULL;

_SvApiLibInitialize InitializeLib = NULL;
_SvApiLibUnInitialize DeinitializeLib = NULL;

// CPU
_SvReadMsr Rdmsr = NULL;
_SvWriteMsr Wrmsr = NULL;
_SvReadPmc Rdpmc = NULL;
_SvCpuid Cpuid = NULL;
_SvReadTsc Rdtsc = NULL;

// I/O
_SvReadIoPortByteEx ReadIoPortByte = NULL;
_SvReadIoPortWordEx ReadIoPortWord = NULL;
_SvReadIoPortDwordEx ReadIoPortDword = NULL;
_SvWriteIoPortByteEx WriteIoPortByte = NULL;
_SvWriteIoPortWordEx WriteIoPortWord = NULL;
_SvWriteIoPortDwordEx WriteIoPortDword = NULL;

// PCI
_SvReadPciConfigByteEx ReadPciConfigByte = NULL;
_SvReadPciConfigWordEx ReadPciConfigWord = NULL;
_SvReadPciConfigDwordEx ReadPciConfigDword = NULL;
_SvWritePciConfigByteEx WritePciConfigByte = NULL;
_SvWritePciConfigWordEx WritePciConfigWord = NULL;
_SvWritePciConfigDwordEx WritePciConfigDword = NULL;

_SvReadPhysicalMemory ReadPhysicalMemory = NULL;
_SvWritePhysicalMemory WritePhysicalMemory = NULL;

_SvGetBiosInfo GetBiosInfo = NULL;
_SvGetSystemInfo GetSysInfo = NULL;
_SvGetBoardInfo GetBoardInfo = NULL;
_SvGetCPUInfo GetCPUInfo = NULL;
_SvGetMemoryInfo GetMemoryInfo = NULL;

_SvGetPciInfo GetPciInfo = NULL;

_SvGetGpioLevel GetGpioLevel = NULL;
_SvSetGpioLevel SetGpioLevel = NULL;
_SvGetGpioDirection GetGpioDirection = NULL;
_SvSetGpioDirection SetGpioDirection = NULL;
_SvWatchdogStart WatchdogStart = NULL;
_SvWatchdogStop WatchdogStop = NULL;
_SvGetCPUTemp GetCPUTemp = NULL;
_SvGetSIOTemp GetSIOTemp = NULL;
_SvGetFanSpeed GetFanSpeed = NULL;
_SvGetVoltageValue GetVoltageValue = NULL;
_SvSetFanPWM SetFanPWM = NULL;

//SMBus
_SvSmbReadByte	SmbReadByte = NULL;
_SvSmbReadWord	SmbReadWord = NULL;
_SvSmbReadBlock	SmbReadBlock = NULL;
_SvSmbWriteByte	SmbWriteByte = NULL;
_SvSmbWriteWord SmbWriteWord = NULL;
_SvSmbWriteBlock SmbWriteBlock = NULL;

_SvSmbReadByteTransfer SmbReadByteTransfer = NULL;
_SvSmbWriteByteTransfer SmbWriteByteTransfer = NULL;

_SvGetLFPBacklightValue GetLFPBacklightValue = NULL;
_SvSetLFPBacklightValue SetLFPBacklightValue = NULL;

//IIC
_SvIICInit		 SvIICInit = NULL;
_SvIICUnInit	 SvIICUnInit = NULL;
_SvIICReadByte	 SvIICReadByte = NULL;
_SvIICWriteByte  SvIICWriteByte = NULL;
_SvIICReadWord	 SvIICReadWord = NULL;
_SvIICWriteWord  SvIICWriteWord = NULL;
_SvIICReadBlock	 SvIICReadBlock = NULL;
_SvIICWriteBlock SvIICWriteBlock = NULL;

_SvIICReadByteExt	 SvIICReadByteExt = NULL;
_SvIICWriteByteExt  SvIICWriteByteExt = NULL;
_SvIICReadWordExt	 SvIICReadWordExt = NULL;
_SvIICWriteWordExt  SvIICWriteWordExt = NULL;
_SvIICReadBlockExt	 SvIICReadBlockExt = NULL;
_SvIICWriteBlockExt SvIICWriteBlockExt = NULL;

_SvIICReadByteTransfer	 SvIICReadByteTransfer = NULL;
_SvIICReadWordTransfer   SvIICReadWordTransfer = NULL;
_SvIICWriteByteTransfer	 SvIICWriteByteTransfer = NULL;
_SvIICWriteWordTransfer  SvIICWriteWordTransfer = NULL;

//GetPostMessage
_SvGetPostMessage  SvGetPostMessage = NULL;
//-----------------------------------------------------------------------------
//
// Initialize
//
//-----------------------------------------------------------------------------

BOOL InitApiLib(HMODULE *hModule)
{
#ifdef _M_X64
	*hModule = LoadLibrary(_T("SvApiLibx64.dll"));
	//*hModule = LoadLibrary(_T("D:\\program\\dllpath\\SvApiLibx64.dll"));  // Change to you own path if needed.
#else
	*hModule = LoadLibrary(_T("SvApiLib.dll"));
#endif

	if(*hModule == NULL)
	{
		printf("Error: Can't load dynamic library! \n");
		return FALSE;
	}

	//-----------------------------------------------------------------------------
	// GetProcAddress
	//-----------------------------------------------------------------------------
	// DLL
	GetDllVersion =			(_SvGetDllVersion)			GetProcAddress (*hModule, "SvGetDllVersion");
	GetDriverVersion =		(_SvGetDriverVersion)		GetProcAddress (*hModule, "SvGetDriverVersionEx");
	InitializeLib =			(_SvApiLibInitialize)		GetProcAddress (*hModule, "SvApiLibInitialize");
	DeinitializeLib =		(_SvApiLibUnInitialize)		GetProcAddress (*hModule, "SvApiLibUnInitialize");

	// CPU
	Rdmsr =					(_SvReadMsr)				GetProcAddress (*hModule, "SvReadMsr");
	Wrmsr =					(_SvWriteMsr)				GetProcAddress (*hModule, "SvWriteMsr");
	Rdpmc =					(_SvReadPmc)				GetProcAddress (*hModule, "SvReadPmc");
	Cpuid =					(_SvCpuid)					GetProcAddress (*hModule, "SvCpuid");
	Rdtsc =					(_SvReadTsc)				GetProcAddress (*hModule, "SvReadTsc");

	// I/O
	ReadIoPortByte =		(_SvReadIoPortByteEx)		GetProcAddress (*hModule, "SvReadIoPortByteEx");
	ReadIoPortWord =		(_SvReadIoPortWordEx)		GetProcAddress (*hModule, "SvReadIoPortWordEx");
	ReadIoPortDword =		(_SvReadIoPortDwordEx)		GetProcAddress (*hModule, "SvReadIoPortDwordEx");
	WriteIoPortByte =		(_SvWriteIoPortByteEx)		GetProcAddress (*hModule, "SvWriteIoPortByteEx");
	WriteIoPortWord =		(_SvWriteIoPortWordEx)		GetProcAddress (*hModule, "SvWriteIoPortWordEx");
	WriteIoPortDword =	    (_SvWriteIoPortDwordEx)		GetProcAddress (*hModule, "SvWriteIoPortDwordEx");

	// PCI
	GetPciInfo          =   (_SvGetPciInfo)             GetProcAddress(*hModule, "SvGetPciInfo");
	ReadPciConfigByte =	(_SvReadPciConfigByteEx)	GetProcAddress (*hModule, "SvReadPciConfigByteEx");
	ReadPciConfigWord =	(_SvReadPciConfigWordEx)	GetProcAddress (*hModule, "SvReadPciConfigWordEx");
	ReadPciConfigDword =	(_SvReadPciConfigDwordEx)	GetProcAddress (*hModule, "SvReadPciConfigDwordEx");
	WritePciConfigByte =	(_SvWritePciConfigByteEx)	GetProcAddress (*hModule, "SvWritePciConfigByteEx");
	WritePciConfigWord =	(_SvWritePciConfigWordEx)	GetProcAddress (*hModule, "SvWritePciConfigWordEx");
	WritePciConfigDword =	(_SvWritePciConfigDwordEx)	GetProcAddress (*hModule, "SvWritePciConfigDwordEx");

	// PhysicalMemory
	ReadPhysicalMemory =	(_SvReadPhysicalMemory)		GetProcAddress (*hModule, "SvReadPhysicalMemory");
	WritePhysicalMemory =	(_SvWritePhysicalMemory)	GetProcAddress (*hModule, "SvWritePhysicalMemory");

	// SMBIOS
	GetBiosInfo =			(_SvGetBiosInfo)	GetProcAddress(*hModule, "SvGetBiosInfo");
	GetSysInfo  =			(_SvGetSystemInfo)GetProcAddress(*hModule, "SvGetSystemInfo");
	GetBoardInfo =			(_SvGetBoardInfo)GetProcAddress(*hModule, "SvGetBoardInfo");
	GetCPUInfo =			(_SvGetCPUInfo)GetProcAddress(*hModule, "SvGetCPUInfo");
	GetMemoryInfo =			(_SvGetMemoryInfo)GetProcAddress(*hModule, "SvGetMemoryInfo");

	// GPIO
	GetGpioLevel =			(_SvGetGpioLevel)GetProcAddress(*hModule, "SvGetGpioLevel");
	SetGpioLevel =			(_SvSetGpioLevel)GetProcAddress(*hModule, "SvSetGpioLevel");
	GetGpioDirection =		(_SvGetGpioDirection)GetProcAddress(*hModule, "SvGetGpioDirection");
	SetGpioDirection =		(_SvSetGpioDirection)GetProcAddress(*hModule, "SvSetGpioDirection");
	
	//Watchdog
	WatchdogStart =			(_SvWatchdogStart)GetProcAddress(*hModule, "SvWatchdogStart");
	WatchdogStop =			(_SvWatchdogStop)GetProcAddress(*hModule, "SvWatchdogStop");
	
	// HW Monitor
	GetCPUTemp =			(_SvGetCPUTemp)GetProcAddress(*hModule, "SvGetCPUTemp");
	GetSIOTemp =			(_SvGetSIOTemp)GetProcAddress(*hModule, "SvGetSIOTemp");
	GetFanSpeed =			(_SvGetFanSpeed)GetProcAddress(*hModule, "SvGetFanSpeed");
	GetVoltageValue =		(_SvGetVoltageValue)GetProcAddress(*hModule, "SvGetVoltageValue");
	SetFanPWM =				(_SvSetFanPWM)GetProcAddress(*hModule, "SvSetFanPWM");

	// SMBUS
	SmbReadByte = (_SvSmbReadByte)GetProcAddress(*hModule, "SvSmbReadByte");
	SmbReadWord = (_SvSmbReadWord)GetProcAddress(*hModule, "SvSmbReadWord");
	SmbReadBlock = (_SvSmbReadBlock)GetProcAddress(*hModule, "SvSmbReadBlock");
	SmbWriteByte = (_SvSmbWriteByte)GetProcAddress(*hModule, "SvSmbWriteByte");
	SmbWriteWord = (_SvSmbWriteWord)GetProcAddress(*hModule, "SvSmbWriteWord");
	SmbWriteBlock = (_SvSmbWriteBlock)GetProcAddress(*hModule, "SvSmbWriteBlock");

	SmbReadByteTransfer = (_SvSmbReadByteTransfer)GetProcAddress(*hModule, "SvSmbReadByteTransfer");
	SmbWriteByteTransfer = (_SvSmbWriteByteTransfer)GetProcAddress(*hModule, "SvSmbWriteByteTransfer");

	// Backlight
	GetLFPBacklightValue = (_SvGetLFPBacklightValue)GetProcAddress(*hModule, "SvGetLFPBacklightValue");
	SetLFPBacklightValue = (_SvSetLFPBacklightValue)GetProcAddress(*hModule, "SvSetLFPBacklightValue");

	//IIC
	SvIICInit = (_SvIICInit)GetProcAddress(*hModule, "SvIICInit");
	SvIICUnInit = (_SvIICUnInit)GetProcAddress(*hModule, "SvIICUnInit");
	SvIICReadByte = (_SvIICReadByte)GetProcAddress(*hModule, "SvIICReadByte");
	SvIICWriteByte = (_SvIICWriteByte)GetProcAddress(*hModule, "SvIICWriteByte");
	SvIICReadWord = (_SvIICReadWord)GetProcAddress(*hModule, "SvIICReadWord");
	SvIICWriteWord = (_SvIICWriteWord)GetProcAddress(*hModule, "SvIICWriteWord");
	SvIICReadBlock = (_SvIICReadBlock)GetProcAddress(*hModule, "SvIICReadBlock");
	SvIICWriteBlock = (_SvIICWriteBlock)GetProcAddress(*hModule, "SvIICWriteBlock");

	SvIICReadByteExt = (_SvIICReadByteExt)GetProcAddress(*hModule, "SvIICReadByteExt");
	SvIICWriteByteExt = (_SvIICWriteByteExt)GetProcAddress(*hModule, "SvIICWriteByteExt");
	SvIICReadWordExt = (_SvIICReadWordExt)GetProcAddress(*hModule, "SvIICReadWordExt");
	SvIICWriteWordExt = (_SvIICWriteWordExt)GetProcAddress(*hModule, "SvIICWriteWordExt");
	SvIICReadBlockExt = (_SvIICReadBlockExt)GetProcAddress(*hModule, "SvIICReadBlockExt");
	SvIICWriteBlockExt = (_SvIICWriteBlockExt)GetProcAddress(*hModule, "SvIICWriteBlockExt");

	SvIICReadByteTransfer = (_SvIICReadByteTransfer)GetProcAddress(*hModule, "SvIICReadByteTransfer");
	SvIICReadWordTransfer = (_SvIICReadWordTransfer)GetProcAddress(*hModule, "SvIICReadWordTransfer");
	SvIICWriteByteTransfer = (_SvIICWriteByteTransfer)GetProcAddress(*hModule, "SvIICWriteByteTransfer");
	SvIICWriteWordTransfer = (_SvIICWriteWordTransfer)GetProcAddress(*hModule, "SvIICWriteWordTransfer");

	SvGetPostMessage = (_SvGetPostMessage)GetProcAddress(*hModule, "SvGetPostMessage");
	//-----------------------------------------------------------------------------
	// Check Functions
	//-----------------------------------------------------------------------------
	if(!(
		GetDllVersion
	&&	GetDriverVersion
	&&	InitializeLib
	&&	DeinitializeLib
	&&	Rdmsr
	&&	Wrmsr
	&&	Rdpmc
	&&	Cpuid
	&&	Rdtsc
	&&	ReadIoPortByte
	&&	ReadIoPortWord
	&&	ReadIoPortDword
	&&	WriteIoPortByte
	&&	WriteIoPortWord
	&&	WriteIoPortDword
	&&  GetPciInfo
	&&	ReadPciConfigByte
	&&	ReadPciConfigWord
	&&  ReadPciConfigDword
	&&	WritePciConfigByte
	&&	WritePciConfigWord
	&&	WritePciConfigDword
	&&	ReadPhysicalMemory
	&&	WritePhysicalMemory
	&&	GetBiosInfo
	&&  GetSysInfo
	&&  GetBoardInfo
	&&  GetCPUInfo
	&&  GetMemoryInfo
	&&  GetGpioLevel
	&&  SetGpioLevel
	&&  GetGpioDirection
	&&  SetGpioDirection
	&&  WatchdogStart
	&&  WatchdogStop
	&&  GetCPUTemp
	&&  GetSIOTemp
	&&  GetFanSpeed
	&&  GetVoltageValue
	&&  SetFanPWM

	&& SmbReadByte
	&& SmbReadWord
	&& SmbReadBlock
	&& SmbWriteByte
	&& SmbWriteWord
	&& SmbWriteBlock
	&& SmbReadByteTransfer
	&& SmbWriteByteTransfer

	&&  GetLFPBacklightValue
	&&  SetLFPBacklightValue

	&& SvIICInit
	&& SvIICUnInit
	&& SvIICReadByte
	&& SvIICWriteByte
	&& SvIICReadWord
	&& SvIICWriteWord
	&& SvIICReadBlock
	&& SvIICWriteBlock
	&& SvIICReadByteExt
	&& SvIICWriteByteExt
	&& SvIICReadWordExt
	&& SvIICWriteWordExt
	&& SvIICReadBlockExt
	&& SvIICWriteBlockExt
	&& SvIICReadByteTransfer
	&& SvIICReadWordTransfer
	&& SvIICWriteByteTransfer
	&& SvIICWriteWordTransfer

	&& SvGetPostMessage
	))
	{
		printf("Error: Can't find export function! \n");
		return FALSE;
	}

	return InitializeLib();
}

//-----------------------------------------------------------------------------
//
// Deinitialize
//
//-----------------------------------------------------------------------------
//KEApiLibInitialize
BOOL DeinitApiLib(HMODULE *hModule)
{
	BOOL result = FALSE;

	if(*hModule == NULL)
	{
		return TRUE;
	}
	else
	{
		DeinitializeLib();
		result = FreeLibrary(*hModule);
		*hModule = NULL;

		return result;
	}
}

BYTE IoRead8(WORD port)
{
	UINT8 Value;
	ReadIoPortByte(port, &Value);
	return Value;
}

WORD IoRead16(WORD port) {
	UINT16 Value;
	ReadIoPortWord(port, &Value);
	return Value;
}

DWORD IoRead32(WORD port) {
	DWORD Value;
	ReadIoPortDword(port, &Value);
	return Value;
}

BOOL IoWrite8(WORD port, BYTE value)
{
	return WriteIoPortByte(port, value);
}

BOOL IoWrite16(WORD port, WORD value)
{
	return WriteIoPortWord(port, value);
}

BOOL IoWrite32(WORD port, DWORD value)
{
	return WriteIoPortDword(port, value);
}

DWORD MmioRead32(DWORD addr)
{
	BYTE *buffer = (BYTE *)malloc(4 * sizeof(BYTE));  // get 4 byte memory
	ReadPhysicalMemory(addr, buffer, 1, 4);
	return *(DWORD*)buffer; //convert to UNIT32
}
VOID MmioWrite32(DWORD addr, DWORD val)
{
	WritePhysicalMemory(addr, (BYTE *)&val, 1, 4);
}


DWORD PciRead32(BYTE bus, BYTE device, BYTE function, DWORD offset)
{
	DWORD Value;
	ReadPciConfigDword(PciBusDevFunc(bus,device,function), offset, &Value);
	return Value;
}

VOID PciWrite32(BYTE bus, BYTE device, BYTE function, DWORD offset, DWORD value)
{
	WritePciConfigDword(PciBusDevFunc(bus, device, function), offset, value);
}






