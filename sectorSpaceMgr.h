/*********************************
 *	Sector Space Manager (SSM)
 *	Author: Michael Lombardi
 **********************************/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gDefinitions.h"
#include "ssmDefinitions.h"

//============================== SSM TYPE DEFINITION ==============================//

typedef struct
{
	FILE *alocMapHandle;
	FILE *freeMapHandle;
	unsigned int sampleCount;
	unsigned char alocMap[SECTOR_BYTES];
	unsigned char freeMap[SECTOR_BYTES];

	unsigned int contSectors;
	unsigned int index[2];

	unsigned int badSector[MAX_INPUT][2];

	float fragmented;
} SecSpaceMgr;

//============================== SSM FUNCTION PROTOTYPES ==========================//

void initSecSpaceMgr(SecSpaceMgr *_ssm);
void initSsmMaps(SecSpaceMgr *_ssm);
Bool allocateSectors(SecSpaceMgr *_ssm);
Bool deallocateSectors(SecSpaceMgr *_ssm);
Bool getSector(int _n, SecSpaceMgr *_ssm);
Bool checkIntegrity(SecSpaceMgr *_ssm);
void isFragmented(SecSpaceMgr *_ssm);
void ssmPrint(SecSpaceMgr *_ssm, int _case, int _startByte);
void setAlocSector(SecSpaceMgr *_ssm, int _byte, int _bit);
void setFreeSector(SecSpaceMgr *_ssm, int _byte, int _bit);

//============================== SSM FUNCTION DEFINITIONS =========================//

void initSecSpaceMgr(SecSpaceMgr *_ssm)
{
	char dbfile[256];
	unsigned int i;

	_ssm->contSectors = 0;
	_ssm->index[0] = (unsigned int)(-1);
	_ssm->index[1] = (unsigned int)(-1);

	for (i = 0; i < MAX_INPUT; i++)
	{
		_ssm->badSector[i][0] = (unsigned int)(-1);
		_ssm->badSector[i][1] = (unsigned int)(-1);
	}

	_ssm->fragmented = 0;
	_ssm->alocMapHandle = Null;
	_ssm->freeMapHandle = Null;

	sprintf(dbfile, "./aMap");
	_ssm->alocMapHandle = fopen(dbfile, "r+");
	sprintf(dbfile, "./fMap");
	_ssm->freeMapHandle = fopen(dbfile, "r+");

	_ssm->sampleCount = fread(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
	_ssm->sampleCount = fread(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);

	fclose(_ssm->alocMapHandle);
	fclose(_ssm->freeMapHandle);
}

void initSsmMaps(SecSpaceMgr *_ssm)
{

	unsigned int i;
	char dbfile[256];

	unsigned char map[SECTOR_BYTES];

	_ssm->alocMapHandle = Null;
	_ssm->freeMapHandle = Null;

	sprintf(dbfile, "./aMap");
	_ssm->alocMapHandle = fopen(dbfile, "r+");
	sprintf(dbfile, "./fMap");
	_ssm->freeMapHandle = fopen(dbfile, "r+");

	for (i = 0; i < SECTOR_BYTES; i++)
	{
		map[i] = 0;
	}

	_ssm->sampleCount = fwrite(map, 1, SECTOR_BYTES, _ssm->alocMapHandle);

	for (i = 0; i < SECTOR_BYTES; i++)
	{
		map[i] = 255;
	}

	_ssm->sampleCount = fwrite(map, 1, SECTOR_BYTES, _ssm->freeMapHandle);

	fclose(_ssm->alocMapHandle);
	fclose(_ssm->freeMapHandle);
}

Bool allocateSectors(SecSpaceMgr *_ssm)
{
	if (_ssm->index[0] != (unsigned int)(-1))
	{
		int n;
		unsigned int byte;
		int bit;

		bit = _ssm->index[1];
		byte = _ssm->index[0];
		n = _ssm->contSectors;

		while (n > 0)
		{
			for (; bit < 8; bit++)
			{
				_ssm->freeMap[byte] -= pow(2, bit);
				_ssm->alocMap[byte] += pow(2, bit);

				n--;
				if (n == 0)
				{
					break;
				}
			}
			byte++;
			bit = 0;
		}
	}

	Bool integrity;
	integrity = checkIntegrity(_ssm);
	if (integrity == False)
		return False;

	_ssm->index[0] = (unsigned int)(-1);
	_ssm->index[1] = (unsigned int)(-1);
	_ssm->contSectors = 0;

	char dbfile[256];

	_ssm->alocMapHandle = Null;
	_ssm->freeMapHandle = Null;

	sprintf(dbfile, "./aMap");
	_ssm->alocMapHandle = fopen(dbfile, "r+");
	sprintf(dbfile, "./fMap");
	_ssm->freeMapHandle = fopen(dbfile, "r+");

	_ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
	_ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);

	fclose(_ssm->alocMapHandle);
	fclose(_ssm->freeMapHandle);

	return True;
}

Bool deallocateSectors(SecSpaceMgr *_ssm)
{
	if (_ssm->index[0] != (unsigned int)(-1))
	{
		int n;
		unsigned int byte;
		int bit;

		bit = _ssm->index[1];
		byte = _ssm->index[0];
		n = _ssm->contSectors;

		while (n > 0)
		{
			for (; bit < 8; bit++)
			{
				_ssm->freeMap[byte] += pow(2, bit);
				_ssm->alocMap[byte] -= pow(2, bit);

				n--;
				if (n == 0)
				{
					break;
				}
			}
			byte++;
			bit = 0;
		}
	}

	Bool integrity;
	integrity = checkIntegrity(_ssm);
	if (integrity == False)
		return False;

	_ssm->index[0] = (unsigned int)(-1);
	_ssm->index[1] = (unsigned int)(-1);
	_ssm->contSectors = 0;

	char dbfile[256];

	_ssm->alocMapHandle = Null;
	_ssm->freeMapHandle = Null;

	sprintf(dbfile, "./aMap");
	_ssm->alocMapHandle = fopen(dbfile, "r+");
	sprintf(dbfile, "./fMap");
	_ssm->freeMapHandle = fopen(dbfile, "r+");

	_ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
	_ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);

	fclose(_ssm->alocMapHandle);
	fclose(_ssm->freeMapHandle);

	return True;
}

Bool getSector(int _n, SecSpaceMgr *_ssm)
{
	int i;
	unsigned int mask;
	unsigned int result;
	unsigned int buff;

	unsigned char *freeMap;
	unsigned long long int subsetMap[1];
	unsigned int byteCount;
	Bool found;

	_ssm->contSectors = _n;

	// We assume 0 < _n < 33
	if (_n < 33 && _n > 0)
	{

		mask = 0;

		for (i = _n - 1; i > -1; i--)
		{
			mask += (unsigned int)pow(2, i);
		}

		found = False;
		result = mask;
		buff = mask;
		subsetMap[0] = 0;
		byteCount = 0;
		freeMap = _ssm->freeMap;

		_ssm->index[0] = -1;
		_ssm->index[1] = -1;

		while (byteCount < SECTOR_BYTES)
		{

			subsetMap[0] = *((unsigned long long int *)freeMap);

			for (i = 0; i < 32; i++)
			{

				buff = (unsigned int)subsetMap[0];

				result = mask & buff;

				if (result == mask)
				{
					found = True;
					break;
				}
				else
					subsetMap[0] >>= 1;
			}

			if (found == True)
			{
				_ssm->index[0] = byteCount + (i / 8);
				_ssm->index[1] = i % 8;
				return True; // break;
			}

			freeMap += 4;
			byteCount += 4;
		}
	}
	return False;
}

Bool checkIntegrity(SecSpaceMgr *_ssm)
{
	unsigned int i, j;
	int bitShift;
	unsigned char result;

	for (i = 0; i < MAX_INPUT; i++)
	{
		_ssm->badSector[i][0] = -1;
		_ssm->badSector[i][1] = -1;
	}

	j = 0;
	for (i = 0; i < SECTOR_BYTES; i++)
	{
		result = _ssm->freeMap[i] ^ _ssm->alocMap[i];

		if (result < 255)
		{
			bitShift = 0;

			while (result % 2 == 1)
			{
				bitShift += 1;
				result >>= 1;
			}

			_ssm->badSector[j][0] = i;
			_ssm->badSector[j][1] = bitShift;

			j++;
		}
	}

	if (_ssm->badSector[0][0] != (unsigned int)(-1))
	{
		return False;
	}

	return True;
}

void isFragmented(SecSpaceMgr *_ssm)
{
	int i, j;
	int tmp;
	int result;
	int fragment;
	unsigned char map[SECTOR_BYTES];

	for (i = 0; i < SECTOR_BYTES; i++)
	{
		map[i] = _ssm->freeMap[i];
	}

	result = map[0] % 2;
	tmp = result;
	fragment = 0;
	_ssm->fragmented = 0;

	for (i = 0; i < SECTOR_BYTES; i++)
	{
		for (j = 0; j < 8; j++)
		{
			map[i] >>= 1;
			result = map[i] % 2;
			if (result != tmp)
			{
				tmp = result;
				fragment++;
			}
		}
	}

	_ssm->fragmented = (float)((float)fragment / (float)NUM_SECTORS);
}

void setAlocSector(SecSpaceMgr *_ssm, int _byte, int _bit)
{
	unsigned char mapByte;
	mapByte = _ssm->alocMap[_byte];

	mapByte >>= (7 - _bit);

	if (mapByte % 2 == 0)
		_ssm->alocMap[_byte] += pow(2, _bit);
	else
		_ssm->alocMap[_byte] -= pow(2, _bit);

	char dbfile[256];

	_ssm->alocMapHandle = Null;
	_ssm->freeMapHandle = Null;

	sprintf(dbfile, "./aMap");
	_ssm->alocMapHandle = fopen(dbfile, "r+");
	sprintf(dbfile, "./fMap");
	_ssm->freeMapHandle = fopen(dbfile, "r+");

	_ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
	_ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);

	fclose(_ssm->alocMapHandle);
	fclose(_ssm->freeMapHandle);
}

void setFreeSector(SecSpaceMgr *_ssm, int _byte, int _bit)
{
	unsigned char mapByte;
	mapByte = _ssm->freeMap[_byte];

	mapByte >>= (7 - _bit);

	if (mapByte % 2 == 0)
		_ssm->freeMap[_byte] += pow(2, _bit);
	else
		_ssm->freeMap[_byte] -= pow(2, _bit);

	char dbfile[256];

	_ssm->alocMapHandle = Null;
	_ssm->freeMapHandle = Null;

	sprintf(dbfile, "./aMap");
	_ssm->alocMapHandle = fopen(dbfile, "r+");
	sprintf(dbfile, "./fMap");
	_ssm->freeMapHandle = fopen(dbfile, "r+");

	_ssm->sampleCount = fwrite(_ssm->alocMap, 1, SECTOR_BYTES, _ssm->alocMapHandle);
	_ssm->sampleCount = fwrite(_ssm->freeMap, 1, SECTOR_BYTES, _ssm->freeMapHandle);

	fclose(_ssm->alocMapHandle);
	fclose(_ssm->freeMapHandle);
}

void ssmPrint(SecSpaceMgr *_ssm, int _case, int _startByte)
{
	int i, j, k;
	int count;
	int sector;
	unsigned char tmp;
	char byteArray[8];

	switch (_case)
	{
	case 0:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Initializing SSM maps...\n");
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 1:
		// printf("DEBUG_LEVEL > 0:\n");
		printf("//Print 128 contiguous Sectors from Free/Aloc Map starting at Sector (%d).\n", _startByte * 8);
		// printf("//P:%d\n\n",_startByte);
		printf("=======================================================================\n");
		printf("FREE MAP\n");

		k = _startByte;
		count = 0;
		for (i = _startByte; i < SECTOR_BYTES && i < _startByte + 16; i++)
		{
			tmp = _ssm->freeMap[i];

			for (j = 0; j < 8; j++)
			{
				if (tmp % 2 == 1)
					byteArray[j] = '1';
				else
					byteArray[j] = '0';
				tmp /= 2;
			}

			for (j = 0; j < 8; j++)
				printf("%c", byteArray[j]);

			printf(" ");

			if ((count + 1) % 8 == 0 || i + 1 == SECTOR_BYTES)
			{
				printf("\n");
				for (j = 0; j < 8; j++)
				{
					if (k * 8 < 10)
						printf("%d        ", k * 8);
					else if (k * 8 < 100)
						printf("%d       ", k * 8);
					else if (k * 8 < 1000)
						printf("%d      ", k * 8);
					else if (k * 8 < 10000)
						printf("%d     ", k * 8);
					else if (k * 8 < 100000)
						printf("%d    ", k * 8);
					else if (k * 8 < 1000000)
						printf("%d   ", k * 8);
					else if (k * 8 < 10000000)
						printf("%d  ", k * 8);
					else if (k * 8 < 100000000)
						printf("%d ", k * 8);
					k++;
				}
				printf("\n");
			}
			count++;
		}
		printf("\n");

		printf("ALLOCATED MAP\n");

		k = _startByte;
		count = 0;
		for (i = _startByte; i < SECTOR_BYTES && i < _startByte + 16; i++)
		{
			tmp = _ssm->alocMap[i];

			for (j = 0; j < 8; j++)
			{
				if (tmp % 2 == 1)
					byteArray[j] = '1';
				else
					byteArray[j] = '0';
				tmp /= 2;
			}

			for (j = 0; j < 8; j++)
				printf("%c", byteArray[j]);

			printf(" ");

			if ((count + 1) % 8 == 0 || i + 1 == SECTOR_BYTES)
			{
				printf("\n");
				for (j = 0; j < 8; j++)
				{
					if (k * 8 < 10)
						printf("%d        ", k * 8);
					else if (k * 8 < 100)
						printf("%d       ", k * 8);
					else if (k * 8 < 1000)
						printf("%d      ", k * 8);
					else if (k * 8 < 10000)
						printf("%d     ", k * 8);
					else if (k * 8 < 100000)
						printf("%d    ", k * 8);
					else if (k * 8 < 1000000)
						printf("%d   ", k * 8);
					else if (k * 8 < 10000000)
						printf("%d  ", k * 8);
					else if (k * 8 < 100000000)
						printf("%d ", k * 8);
					k++;
				}
				printf("\n");
			}
			count++;
		}
		printf("=======================================================================\n\n\n");
		break;
	case 2:
		printf("DEBUG_LEVEL > 0:\n");
		sector = 8 * _ssm->index[0] + _ssm->index[1];
		printf("//Allocate %d contiguous sectors.\n", _ssm->contSectors);
		printf("//A:%d\n\n", _ssm->contSectors);
		printf("Allocating %d contiguous sectors at sector %d...\n", _ssm->contSectors, sector);
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 3:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Initializing SSM...\n");
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 4:
		printf("DEBUG_LEVEL > 0:\n");
		for (k = 0; k < MAX_INPUT; k++)
		{
			if (_ssm->badSector[k][0] == (unsigned int)(-1))
			{
				break;
			}
			sector = 8 * _ssm->badSector[k][0] + _ssm->badSector[k][1];
			printf("Failed to allocate sectors at sector %d\n", sector);
		}
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 5:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Check for degree of fragmentation.\n");
		printf("//F\n\n");
		printf("The degree of memory fragmentation is %10.10f \n", _ssm->fragmented);
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 6:
		printf("DEBUG_LEVEL > 0:\n");
		sector = 8 * _ssm->index[0] + _ssm->index[1];
		printf("//Deallocate %d contiguous sectors starting at sector %d\n", _ssm->contSectors, _ssm->index[0] * 8 + _ssm->index[1]);
		printf("//D:%d:%d:%d\n\n", _ssm->contSectors, _ssm->index[0], _ssm->index[1]);
		printf("Deallocating %d contiguous sectors at sector %d...\n", _ssm->contSectors, sector);
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 7:
		printf("DEBUG_LEVEL > 0:\n");
		for (k = 0; k < MAX_INPUT; k++)
		{
			if (_ssm->badSector[k][0] == (unsigned int)(-1))
			{
				break;
			}
			sector = 8 * _ssm->badSector[k][0] + _ssm->badSector[k][1];
			printf("Failed to deallocate sector %d\n", sector);
		}
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 8:
		printf("DEBUG_LEVEL > 0:\n");
		for (k = 0; k < MAX_INPUT; k++)
		{
			if (_ssm->badSector[k][0] == (unsigned int)(-1))
			{
				break;
			}
			sector = 8 * _ssm->badSector[k][0] + _ssm->badSector[k][1];
			printf("Failed integrity check at sector %d\n", sector);
		}
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
		printf("\n");
		break;
	case 9:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Passed integrity check.\n");
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 10:
		printf("DEBUG_LEVEL > 0:\n");
		printf("//Check for integrity.\n");
		printf("//I\n\n");
		printf("Checking integrity of the maps...\n");
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 11:
		printf("DEBUG_LEVEL > 0:\n");
		printf("The memory sectors are %f percent fragmented.\n", _ssm->fragmented);
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 12:
		printf("DEBUG_LEVEL > 0:\n");
		sector = 8 * _ssm->index[0] + _ssm->index[1];
		printf("//Set allocated map sector (%d*8 + %d).\n", _ssm->index[0], _ssm->index[1]);
		printf("//X:%d:%d\n\n", _ssm->index[0], _ssm->index[1]);
		printf("Setting allocated map sector %d\n", sector);
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 13:
		printf("DEBUG_LEVEL > 0:\n");
		sector = 8 * _ssm->index[0] + _ssm->index[1];
		printf("//Set free map sector (%d*8 + %d).\n", _ssm->index[0], _ssm->index[1]);
		printf("//Y:%d:%d\n\n", _ssm->index[0], _ssm->index[1]);
		printf("Setting free map sector %d\n", sector);
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 14:
		printf("\nEND");
		break;
	case 15:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Bad input...\n");
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 16:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Could not find %d contiguous sectors.\n", _ssm->contSectors);
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 17:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Reading input file...\n");
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 18:
		printf("DEBUG_LEVEL > 0:\n");
		printf("Creating stub output...\n");
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 19:
		printf("DEBUG_LEVEL > 1:\n");
		printf("Variable information:\n");
		printf("contSectors = %d\n", _ssm->contSectors);
		printf("index[0] = %d\n", _ssm->index[0]);
		printf("index[1] = %d\n", _ssm->index[1]);
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	case 20:
		printf("DEBUG_LEVEL > 0:\n");
		sector = 8 * _ssm->index[0] + _ssm->index[1];
		printf("//Get %d contiguous sectors.\n", _ssm->contSectors);
		printf("//G:%d\n\n", _ssm->contSectors);
		printf("There are %d contiguous sectors at sector %d.\n", _ssm->contSectors, sector);
		printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n\n");
		break;
	}
}
