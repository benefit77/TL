//-----------------------------------------------------------------------------
//     Author : Seavo Software Research Department
//       Mail : bcc@seavo.com
//        Web : https://www.seavo.com/
//    License : The modified BSD license
//
//    Copyright 2003-2022 seavo.com. All rights reserved.
//-----------------------------------------------------------------------------

#include "SvApiLib.h"
#include <stdio.h>
#include <string.h>
typedef unsigned char BYTE;

void printfHelp(char *argv0)
{
        printf("Ver: 20221223\n\n");
	printf("<%s> <-w/-r> <9539/9554/9555> <address> <GPI/GPO> <IONUM> <0/1>\n", argv0);
	printf("For Example:\n");
	printf("%s -w 9554 0x42 GPO 7 1 \n", argv0);
	printf("%s -r 9554 0x42 GPI 2 \n", argv0);
	printf("\n%s -h:for help!\n", argv0);
}

bool Tca9539_GPO_Write(UINT8 address, UINT8 bit, UINT8 value)
{
	UINT8 temp;

	if (bit >= 0 && bit <= 7)
	{
		temp = ((SvSmbReadByte(address, 0x02) & (~(1 << bit))) | (value << bit));
		SvSmbWriteByte(address, 0x02, temp);
	}
	else if (bit >= 8 && bit <= 15)
	{
		bit = bit - 8;
		temp = (SvSmbReadByte(address, 0x03) & (~(1 << bit))) | (value << bit);
		SvSmbWriteByte(address, 0x03, temp);
	}
	else
	{
		printf("parameter error! please input 0~15.\n");
		return 0;
	}

	return 1;
}

bool Tca9554_GPO_Write(UINT8 address, UINT8 bit, UINT8 value)
{
	UINT8 temp;

	if (bit >= 0 && bit <= 7)
	{
		temp = (SvSmbReadByte(address, 0x01) & (~(1 << bit))) | (value << bit);
		SvSmbWriteByte(address, 0x01, temp);
	}
	else
	{
		printf("parameter error! please input 0~7.\n");
		return 0;
	}

	return 1;
}

bool Tca9555_GPO_Write(UINT8 address, UINT8 bit, UINT8 value)
{
	UINT8 temp;

	if (bit >= 0 && bit <= 7)
	{
		temp = (SvSmbReadByte(address, 0x02) & (~(1 << bit))) | (value << bit);
		SvSmbWriteByte(address, 0x02, temp);
	}
	else if (bit >= 8 && bit <= 15)
	{
		bit = bit - 8;
		temp = (SvSmbReadByte(address, 0x03) & (~(1 << bit))) | (value << bit);
		SvSmbWriteByte(address, 0x03, temp);
	}
	else
	{
		printf("parameter error! please input 0~15.\n");
		return 0;
	}

	return 1;
}

BYTE Tca9539_GPO_Read(UINT8 address, UINT8 bit)
{
	UINT8 BYTEVal, temp;

	if (bit >= 0 && bit <= 7)
	{
		BYTEVal = SvSmbReadByte(address, 0x02);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPO\t");
		printf("%lx,GPIO%d is %x\n\n", address, bit, temp);
		return temp;
	}
	else if (bit >= 8 && bit <= 15)
	{
		bit = bit - 8;
		BYTEVal = SvSmbReadByte(address, 0x03);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPO\t");
		printf("%lx,GPIO%d is %x\n\n", address, (bit+8), temp);
		return temp;
	}
	else
	{
		printf("parameter error! please input 0~15.\n");
		return -1;
	}
}

BYTE Tca9554_GPO_Read(UINT8 address, UINT8 bit)
{
	UINT8 BYTEVal, temp;

	if (bit >= 0 && bit <= 7)
	{
		BYTEVal = SvSmbReadByte(address, 0x01);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPO\t");
		printf("%lx,GPIO%d is %x\n\n", address, bit, temp);
		return temp;
	}
	else
	{
		printf("parameter error! please input 0~7.\n");
		return -1;
	}
}

BYTE Tca9555_GPO_Read(UINT8 address, UINT8 bit)
{
	UINT8 BYTEVal, temp;

	if (bit >= 0 && bit <= 7)
	{
		BYTEVal = SvSmbReadByte(address, 0x02);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPO\t");
		printf("%lx,GPIO%d is %x\n\n", address, bit, temp);
		return temp;
	}
	else if (bit >= 8 && bit <= 15)
	{
		bit = bit - 8;
		BYTEVal = SvSmbReadByte(address, 0x03);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPO\t");
		printf("%lx,GPIO%d is %x\n\n", address, (bit+8), temp);
		return temp;
	}
	else
	{
		printf("parameter error! please input 0~15.\n");
		return -1;
	}
}

BYTE Tca9539_GPI_Read(UINT8 address, UINT8 bit)
{
	UINT8 BYTEVal, temp;

	if (bit >= 0 && bit <= 7)
	{
		BYTEVal = SvSmbReadByte(address, 0x00);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPI\t");
		printf("%lx,GPIO%d is %x\n\n", address, bit, temp);
		return temp;
	}
	else if (bit >= 8 && bit <= 15)
	{
		bit = bit - 8;
		BYTEVal = SvSmbReadByte(address, 0x01);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPI\t");
		printf("%lx,GPIO%d is %x\n\n", address, (bit+8), temp);
		return temp;
	}
	else
	{
		printf("parameter error! please input 0~15.\n");
		return -1;
	}
}

BYTE Tca9554_GPI_Read(UINT8 address, UINT8 bit)
{
	UINT8 BYTEVal, temp;

	if (bit >= 0 && bit <= 7)
	{
		BYTEVal = SvSmbReadByte(address, 0x00);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPI\t");
		printf("%lx,GPIO%d is %x\n\n", address, bit, temp);
		return temp;
	}
	else
	{
		printf("parameter error! please input 0~7.\n");
		return -1;
	}
}

BYTE Tca9555_GPI_Read(UINT8 address, UINT8 bit)
{
	UINT8 BYTEVal, temp;

	if (bit >= 0 && bit <= 7)
	{
		BYTEVal = SvSmbReadByte(address, 0x00);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPI\t");
		printf("%lx,GPIO%d is %x\n\n", address, bit, temp);
		return temp;
	}
	else if (bit >= 8 && bit <= 15)
	{
		bit = bit - 8;
		BYTEVal = SvSmbReadByte(address, 0x01);
		temp = ((BYTEVal >> bit) & 0x1);
		printf("Reading Data at address,GPI\t");
		printf("%lx,GPIO%d is %x\n\n", address, (bit+8), temp);
		return temp;
	}
	else
	{
		printf("parameter error! please input 0~15.\n");
		return -1;
	}
}

int main(int argc, char* argv[])
{
	UINT8 address = 0x70, bit = 0, value = 0, comValue = 0;

	if(!SvApiLibInit())
	{
		printf("Initialize Driver Error...\n\n");
		return 0;
	}
	printf("Initialize Driver OK...\n\n");

	if (argv[1] == NULL || !strcmp(argv[1], "-h"))
	{
		printfHelp(argv[0]);
		return 0;
	}

	if (!strcmp(argv[1], "-w")) //write
	{
		address = (UINT8)strtoul(argv[3], 0, 16);
		bit = (UINT8)strtoul(argv[5], NULL, 10);
		value = (UINT8)strtoul(argv[6], 0, 16);

		if (!strcmp(argv[4], "GPO"))//GPI can't write
		{
			if (!strcmp(argv[2], "9539"))
			{
				Tca9539_GPO_Write(address, bit, value);
			}
			else if (!strcmp(argv[2], "9554"))
			{
				Tca9554_GPO_Write(address, bit, value);
			}
			else if (!strcmp(argv[2], "9555"))
			{
				Tca9555_GPO_Write(address, bit, value);
			}
			else
			{
				printf("parameter error! please input 9539/9554/9555.\n");
			}
		}
		else
		{
			printf("parameter error! GPI can't write.\n");
		}

		printf("write option:\n\tchipset=Tca%s addr=%x,bit=GPO%x,value:%x.\n", argv[2], address, bit, value);
	}
	else if (!strcmp(argv[1], "-r")) //read
	{
		address = (UINT8)strtoul(argv[3], 0, 16);
		bit = (UINT8)strtoul(argv[5], NULL, 10);

		if (!strcmp(argv[4], "GPO"))
		{
			if (!strcmp(argv[2], "9539"))
			{
				Tca9539_GPO_Read(address, bit);
			}
			else if (!strcmp(argv[2], "9554"))
			{
				Tca9554_GPO_Read(address, bit);
			}
			else if (!strcmp(argv[2], "9555"))
			{
				Tca9555_GPO_Read(address, bit);
			}
			else
			{
				printf("parameter error! \n");
			}
		}
		else if (!strcmp(argv[4], "GPI"))
		{
			if (!strcmp(argv[2], "9539"))
			{
				Tca9539_GPI_Read(address, bit);
			}
			else if (!strcmp(argv[2], "9554"))
			{
				Tca9554_GPI_Read(address, bit);
			}
			else if (!strcmp(argv[2], "9555"))
			{
				Tca9555_GPI_Read(address, bit);
			}
			else
			{
				printf("parameter error! \n");
			}
		}
		else
		{
			printf("parameter error! \n");
		}
	}
	else
	{
		printfHelp(argv[0]);
		printf("parameter error!\n");
	}

	SvApiLibUnInit();
        return 0;
}
