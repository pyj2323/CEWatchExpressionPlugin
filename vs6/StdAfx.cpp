// stdafx.cpp : source file that includes just the standard includes
//	Honoka.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"




int g_PluginID = -1;
int g_DebugID = -1;
int g_DisassemblerID = -1;
ExportedFunctions CheatEngine;

vector<CWatch*> g_WatchList;

size_t* g_pCurrentDebuggerInterface;

TReadProcessMemory* pCE_ReadProcessMemory;
TWriteProcessMemory* pCE_WriteProcessMemory;
TOpenThread* pCE_OpenThread;

BOOL CE_GetThreadContext(HANDLE hThread, PCONTEXT pCtx, BOOL isFrozenThread) {
	size_t CurrentDebuggerInterface = *g_pCurrentDebuggerInterface;

	if(CurrentDebuggerInterface) {
		size_t temp;
		temp = *(size_t*)CurrentDebuggerInterface;
		temp = *(size_t*)(temp + 0xE8);

		typedef BOOL (*tGTC)(size_t, HANDLE, PCONTEXT, BOOL);

		tGTC GTC = (tGTC)temp;
	
		return GTC(CurrentDebuggerInterface, hThread, pCtx, isFrozenThread);
	}
	
	return GetThreadContext(hThread, pCtx);
}

BOOL CE_SetThreadContext(HANDLE hThread, const PCONTEXT pCtx, BOOL isFrozenThread) {
	size_t CurrentDebuggerInterface = *g_pCurrentDebuggerInterface;

	if (CurrentDebuggerInterface) {
		size_t temp;
		temp = *(size_t*)CurrentDebuggerInterface;
		temp = *(size_t*)(temp + 0xD8);

		typedef BOOL(*tGTC)(size_t, HANDLE, PCONTEXT, BOOL);

		tGTC STC = (tGTC)temp;

		return STC(CurrentDebuggerInterface, hThread, pCtx, isFrozenThread);
	}

	return SetThreadContext(hThread, pCtx);
}