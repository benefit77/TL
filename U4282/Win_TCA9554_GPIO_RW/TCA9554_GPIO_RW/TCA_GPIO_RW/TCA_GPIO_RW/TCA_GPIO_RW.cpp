//-----------------------------------------------------------------------------
//     Author : Seavo Software Research Department
//       Mail : bcc@seavo.com
//        Web : https://www.seavo.com/
//    License : The modified BSD license
//
//    Copyright 2003-2022 seavo.com. All rights reserved.
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <windows.h>
#include <winioctl.h>
#include <string.h>
#include <cstdio>
#include <iostream>
#include "SvApiInit.h"
HMODULE module;

UINT8 smb_err = 0x00;

//Ver 0.0.1
//命令行程序
void printfHelp(char *argv0)
{
	printf("Ver: 20240924\n\n");
	printf("<%s> <-w/-r> <address> <GPI/GPO> <IONUM> <0/1>\n", argv0);
	printf("For Example:\n");
	printf("%s -w 0x72 GPO 7 1  \n", argv0);
	printf("%s -r 0x72 GPI 2  \n", argv0);
	printf("\n%s -h:for help!\n", argv0);
}

int main(int argc, char** argv)
{
	UINT8 address = 0x72, offset = 0, value = 0, comValue = 0;

	if (!InitApiLib(&module))
	{
		printf("Initialize Driver Error...\n\n");
		return	-1;
	}
	printf("Initialize Driver OK...\n\n");

	if (argv[1] == NULL || !strcmp(argv[1], "-h"))
	{
		printfHelp(argv[0]);
		return 0;
	}

	if (!strcmp(argv[1], "-w") && !strcmp(argv[3], "GPO") )//write
	{
		address = (UINT8)strtoul(argv[2], 0, 16);
		offset = (UINT8)strtoul(argv[4], NULL, 16);
		value = (UINT8)strtoul(argv[5], 0, 16);
		SmbReadByte(address, 0x01, &comValue);
	    if(value == 0)
			comValue &= ~(1 << offset);
		else
			comValue |= (1 << offset);
		SmbWriteByte(address, 0x01, comValue);

		SmbReadByte(address, 0x03, &comValue);
		comValue &= ~(1 << offset);
		SmbWriteByte(address, 0x03, comValue);

		printf("write option:\n\taddr=%x,offset=%x,value:%d successful.\n", address, offset, value);
	}
	else if (!strcmp(argv[1], "-r") && !strcmp(argv[3], "GPO")) //read
	{
		address = (UINT8)strtoul(argv[2], 0, 16);
		offset = (UINT8)strtoul(argv[4], NULL, 16);
		SmbReadByte(address, 0x01, &value);
		comValue = (value >> offset) & 0x01;
		printf("read option:\n\taddr=%x,offset=%x,value:%d.\n", address, offset, comValue);
		return comValue;
	}
	else if (!strcmp(argv[1], "-r") && !strcmp(argv[3], "GPI")) //read
	{
		address = (UINT8)strtoul(argv[2], 0, 16);
		offset = (UINT8)strtoul(argv[4], NULL, 16);

		SmbReadByte(address, 0x03, &comValue);
		comValue |= (1 << offset);
		SmbWriteByte(address, 0x03, comValue);

		SmbReadByte(address, 0x00, &value);
		comValue = (value >> offset) & 0x01;
		printf("read option:\n\taddr=%x,offset=%x,value:%d.\n", address, offset, comValue);
		return comValue;
	}
	else
	{
		printfHelp(argv[0]);
		printf("parameter error!\n");
	}
	return 0;
}
