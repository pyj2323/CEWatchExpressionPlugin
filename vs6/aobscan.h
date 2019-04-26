#pragma once

#include <windows.h>
#include <stdio.h>
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")

bool bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
size_t FindPattern(size_t dwAddress, size_t dwLen, BYTE *bMask, const char *szMask);
size_t AobscanRange(size_t dwAddress, size_t dwLen, const char *strPattern);
bool bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
size_t AobscanModule(HMODULE hModule, const char *strPattern);
