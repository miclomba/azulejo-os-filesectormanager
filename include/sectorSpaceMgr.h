/*********************************
 *	Sector Space Manager (SSM)
 *	Author: Michael Lombardi
 **********************************/
#ifndef SECTOR_SPACE_MGR_H
#define SECTOR_SPACE_MGR_H

#include <stdio.h>

#include "config.h"
#include "gDefinitions.h"
#include "ssmDefinitions.h"

//============================== SSM TYPE DEFINITION ==============================//
typedef struct {
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

#endif  // SECTOR_SPACE_MGR_H