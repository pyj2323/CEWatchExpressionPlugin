#pragma once

#include "stdafx.h"
#include "aobscan.h"

size_t AobscanRange(size_t dwAddress, size_t dwLen, const char *strPattern) {
	size_t i = 0, j = 0;

	BYTE bMask[0x100];
	char szMask[0x100];

	size_t bByte;
	while(strPattern[i]) {
		char _c[] = {0x25,0x63,0};				//%c
		char _02X[] = {0x25,0x30,0x32,0x58,0};	//%02X
		char _02x[] = {0x25,0x30,0x32,0x78,0};	//%02x

		bByte = 0;
		sscanf(strPattern + i, _c, &bByte);
		if(bByte == ' ')
		{
			i++;
			continue;
		}
		else if(bByte == '?')
		{
			szMask[j] = '?';
			i++;j++;
			continue;
		}

		if(sscanf(strPattern + i, _02X, &bByte) == 1)
		{
			szMask[j] = 'x';
			bMask[j] = BYTE(bByte);
			i += 2;
			j++;
		}
		else if(sscanf(strPattern + i, _02x, &bByte) == 1)
		{
			szMask[j] = 'x';
			bMask[j] = BYTE(bByte);
			i += 2;
			j++;
		}
		else 
			return 0;
	}

	szMask[j] = 0;

	size_t pagesize = 0x1000;
	size_t pagemask =  ~pagesize + 1;

	size_t start = dwAddress & pagemask;
	size_t end = (dwAddress + dwLen + pagesize) & pagemask;

	for(i = start ; i < end ; i += pagesize) {
		char buffer[0x2000] = {0,};
		SIZE_T Readed;

		if(CE_ReadProcessMemory((HANDLE)-1, (LPCVOID)i, (LPVOID)buffer, pagesize, &Readed) == FALSE)
			continue;	
		CE_ReadProcessMemory((HANDLE)-1, (LPCVOID)(i + pagesize), (LPVOID)(buffer + pagesize), pagesize, &Readed);

		size_t address = (size_t)buffer;
		while(1) {
			size_t dwRet = FindPattern(address, pagesize, bMask, szMask);
			if(dwRet == 0) 
				break;
			else {
				size_t dwResult = dwRet - (size_t)buffer + i;
				if(dwResult >= dwAddress && dwResult < dwAddress + dwLen && dwResult != (size_t)bMask)
					return dwResult;

				address = dwRet + 1;
			}
		}
	}
	return 0;
}


bool bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	for( ; *szMask ; ++szMask, ++pData, ++bMask)
		if(*szMask == 'x' && *pData != *bMask)
			return false;
	return (*szMask) == NULL;
}

size_t FindPattern(size_t dwAddress, size_t dwLen, BYTE *bMask, const char *szMask)
{
	for(size_t i = 0 ; i < dwLen ; i++)
		if(bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (size_t)(dwAddress + i);
	return 0;
}

size_t AobscanModule(HMODULE hModule, const char *strPattern) {
	MODULEINFO modinfo;
	GetModuleInformation((HANDLE)-1, GetModuleHandle(0), &modinfo, sizeof(MODULEINFO));
	return AobscanRange((size_t)hModule, modinfo.SizeOfImage, strPattern);
}
