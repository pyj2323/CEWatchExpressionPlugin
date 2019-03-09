// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.

#include <afxwin.h>         // MFC 핵심 및 표준 구성 요소입니다.
#include <afxext.h>         // MFC 확장입니다.

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE 클래스입니다.
#include <afxodlgs.h>       // MFC OLE 대화 상자 클래스입니다.
#include <afxdisp.h>        // MFC 자동화 클래스입니다.
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC 데이터베이스 클래스입니다.
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO 데이터베이스 클래스입니다.
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // Internet Explorer 4 공용 컨트롤에 대한 MFC 지원입니다.
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // Windows 공용 컨트롤에 대한 MFC 지원입니다.
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "cepluginsdk.h"
#include "resource.h"
#include "Watch.h"

#include "aobscan.h"

#include <windows.h>
#include <stdio.h>

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
using namespace std;


#define WM_GETCONTEXT (WM_USER + 100)


extern int g_PluginID;
extern int g_DebugID;
extern int g_DisassemblerID;
extern ExportedFunctions CheatEngine;

#include "lua530\lua.hpp"
#pragma comment(lib, "lua530.lib")

#if (_MSC_VER >= 1900)
#pragma comment(lib, "legacy_stdio_definitions.lib")
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }
#endif

union EFlags {
	DWORD dwValue;
	struct {
		bool CF			: 1;
		bool reserved0	: 1;
		bool PF			: 1;
		bool reserved1	: 1;
		bool AF			: 1;
		bool reserved2	: 1;
		bool ZF			: 1;
		bool SF			: 1;
		bool TF			: 1;
		bool IF			: 1;
		bool DF			: 1;
		bool OF			: 1;
		bool IOPL0		: 1;
		bool IOPL1		: 1;
		bool NT			: 1;
		bool reserved3	: 1;
		bool RF			: 1;
		bool VM			: 1;
		bool AC			: 1;
		bool VIF		: 1;
		bool VIP		: 1;
		bool ID			: 1;
	};
};

extern vector<CWatch*> g_WatchList;
extern size_t* g_pCurrentDebuggerInterface;

#pragma pack(push, 1)
 
typedef struct {
	BYTE Bytes[8];
}TJclMMRegister;

typedef struct {
	TJclMMRegister MMRegister;
}TJclFPUData;

typedef struct {
	TJclFPUData Data;
	BYTE Reserved[6];
}TJclFPURegister;

typedef struct {
	TJclFPURegister Register[8];
}TJclFPURegisters;

typedef struct {
	BYTE Bytes[16];
}TJclXMMRegister;

typedef struct {
	TJclXMMRegister XMM[16];
}TJclXMMRegisters;

typedef struct {
    WORD FCW;                           // bytes from 0   to 1
    WORD FSW;                           // bytes from 2   to 3
    BYTE FTW;                           // byte 4
    BYTE Reserved1;                     // byte 5
    WORD FOP;                           // bytes from 6   to 7
    DWORD FpuIp;					 // bytes from 8   to 11
    WORD CS;                            // bytes from 12  to 13
    WORD Reserved2;                     // bytes from 14  to 15
    DWORD FpuDp;                     // bytes from 16  to 19
    WORD DS;                            // bytes from 20  to 21
    WORD Reserved3;                     // bytes from 22  to 23
    DWORD MXCSR;                     // bytes from 24  to 27
    DWORD MXCSRMask;                 // bytes from 28  to 31
	TJclFPURegisters FPURegisters;      // bytes from 32  to 159
	TJclXMMRegisters XMMRegisters;      // bytes from 160 to 415
	BYTE Reserved4[511 - 416 + 1];	// bytes from 416 to 511
}TextendedRegisters;

typedef struct {
	UINT64 threadid;
	UINT64 eflags;
	UINT64 rax;
	UINT64 rbx;
	UINT64 rcx;
	UINT64 rdx;
	UINT64 rsi;
	UINT64 rdi;
	UINT64 rbp;
	UINT64 rsp;
	UINT64 rip;
	UINT64 r8;
	UINT64 r9;
	UINT64 r10;
	UINT64 r11;
	UINT64 r12;
	UINT64 r13;
	UINT64 r14;
	UINT64 r15;
	UINT64 cs;
	UINT64 ds;
	UINT64 es;
	UINT64 fs;
	UINT64 gs;
	UINT64 ss;
	UINT64 dr0;
	UINT64 dr1;
	UINT64 dr2;
	UINT64 dr3;
	UINT64 dr6;
	UINT64 dr7;
	TextendedRegisters fxstate;
	UINT64 LBR_Count;
	UINT64 LBR[16];
}TDebuggerState, *PDebuggerState;

#pragma pack(pop)

typedef BOOL(__stdcall *TReadProcessMemory)(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesRead);
typedef BOOL(__stdcall *TWriteProcessMemory)(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesRead);
typedef HANDLE(__stdcall *TOpenThread)(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId);

extern TReadProcessMemory* pCE_ReadProcessMemory;
extern TWriteProcessMemory* pCE_WriteProcessMemory;
extern TOpenThread* pCE_OpenThread;

#define CE_ReadProcessMemory (*pCE_ReadProcessMemory)
#define CE_WriteProcessMemory (*pCE_WriteProcessMemory)
#define CE_OpenThread (*pCE_OpenThread)

#include "parser.h"

BOOL CE_GetThreadContext(HANDLE hThread, PCONTEXT pCtx, BOOL isFrozenThread);
BOOL CE_SetThreadContext(HANDLE hThread, const PCONTEXT pCtx, BOOL isFrozenThread);