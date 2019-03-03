// stdafx.cpp : 표준 포함 파일만 들어 있는 소스 파일입니다.
// Honoka.pch는 미리 컴파일된 헤더가 됩니다.
// stdafx.obj에는 미리 컴파일된 형식 정보가 포함됩니다.

#include "stdafx.h"

int g_PluginID = -1;
int g_DebugID = -1;
int g_DisassemblerID = -1;
ExportedFunctions CheatEngine;
//HANDLE* g_phDevice;

vector<CWatch*> g_WatchList;

size_t* g_pCurrentDebuggerInterface;

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

/*

BOOL DBKDebug_GetDebuggerState(HANDLE hDevice, PCONTEXT pCtx) {
	TDebuggerState state;

	const DWORD FILE_READ_ACCESS = 0x0001;
	const DWORD FILE_WRITE_ACCESS = 0x0002;
	const DWORD FILE_RW_ACCESS = FILE_READ_ACCESS | FILE_WRITE_ACCESS;     
	const DWORD FILE_DEVICE_UNKNOWN = 0x00000022;
	const DWORD IOCTL_UNKNOWN_BASE = FILE_DEVICE_UNKNOWN;  
	const DWORD METHOD_BUFFERED = 0;
	const DWORD IOCTL_CE_GETDEBUGGERSTATE = 
		(IOCTL_UNKNOWN_BASE << 16) |
		(0x0833 << 2) |
		(METHOD_BUFFERED) |
		(FILE_RW_ACCESS << 14); 

	if(hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	FillMemory(&state, sizeof(TDebuggerState), 1);

	DWORD dwBytes;
	if(!DeviceIoControl(hDevice, IOCTL_CE_GETDEBUGGERSTATE, NULL, 0, &state, sizeof(TDebuggerState), &dwBytes, NULL))
		return FALSE;

	pCtx->Rax = state.rax;
	pCtx->Rbx = state.rbx;
	pCtx->Rcx = state.rcx;
	pCtx->Rdx = state.rdx;
	pCtx->Rsi = state.rsi;
	pCtx->Rdi = state.rdi;
	pCtx->Rbp = state.rbp;
	pCtx->Rsp = state.rsp;
	pCtx->Rip = state.rip;
	pCtx->R8 = state.r8;
	pCtx->R9 = state.r9;
	pCtx->R10 = state.r10;
	pCtx->R11 = state.r11;
	pCtx->R12 = state.r12;
	pCtx->R13 = state.r13;
	pCtx->R14 = state.r14;
	pCtx->R15 = state.r15;
	pCtx->SegCs = (WORD)state.cs;
	pCtx->SegSs = (WORD)state.ss;
	pCtx->SegDs = (WORD)state.ds;
	pCtx->SegEs = (WORD)state.es;
	pCtx->SegFs = (WORD)state.fs;
	pCtx->SegGs = (WORD)state.gs;
	pCtx->EFlags = (DWORD)state.eflags;
	pCtx->Dr0 = state.dr0;
	pCtx->Dr1 = state.dr1;
	pCtx->Dr2 = state.dr2;
	pCtx->Dr3 = state.dr3;
	pCtx->Dr6 = state.dr6;
	pCtx->Dr7 = state.dr7;
	CopyMemory(&pCtx->FltSave, &state.fxstate, 512);
	pCtx->ContextFlags = 0;
	return TRUE;
}


*/