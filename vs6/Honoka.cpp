// Honoka.cpp : 해당 DLL의 초기화 루틴을 정의합니다.
#include "stdafx.h"
#include "Honoka.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CHonokaApp
BEGIN_MESSAGE_MAP(CHonokaApp, CWinApp)
END_MESSAGE_MAP()

// CHonokaApp 생성
CHonokaApp::CHonokaApp() {}
CHonokaApp theApp;
BOOL CHonokaApp::InitInstance()
{
	CWinApp::InitInstance();
	return TRUE;
}


int __stdcall debugeventplugin(LPDEBUG_EVENT lpDebugEvent) {
	DWORD dwDebugEventCode = lpDebugEvent->dwDebugEventCode;

	if (dwDebugEventCode != EXCEPTION_DEBUG_EVENT)
		return 0;

	DWORD dwProcessId = lpDebugEvent->dwProcessId;
	DWORD dwThreadId = lpDebugEvent->dwThreadId;

	HANDLE hThread = CE_OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadId);
	if (!hThread)
		return 0;

	CONTEXT ctx = { 0 };
	ctx.ContextFlags = CONTEXT_ALL;

	BOOL bResult = CE_GetThreadContext(hThread, &ctx, TRUE);

	CloseHandle(hThread);

	if (!bResult)
		return 0;

	PVOID pExceptionAddress;

	switch (lpDebugEvent->u.Exception.ExceptionRecord.ExceptionCode) {
	case STATUS_BREAKPOINT:			//Software BP
	case 0x4000001F:			//windows debugger on x86 process
		pExceptionAddress = (PVOID)(ctx.Rip - 1);
		break;
	default:
		pExceptionAddress = (PVOID)(ctx.Rip);
		break;
	}

	EnterCriticalSection(&g_WatchListCS);
	//for(auto pWatch : g_WatchList) {
	for (vector<CWatch*>::iterator it = g_WatchList.begin(); it != g_WatchList.end(); it++) {
		CWatch& WatchDlg = **it;
		if (WatchDlg.m_pAddress != pExceptionAddress)
			continue;
		if (!WatchDlg.m_bCapture)
			continue;

		WatchDlg.AddItem(ctx);
	}
	LeaveCriticalSection(&g_WatchListCS);
	return 0;
}


UINT DlgThread(LPVOID lpParam) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWatch WatchDlg(lpParam);

	EnterCriticalSection(&g_WatchListCS);
	g_WatchList.push_back(&WatchDlg);
	LeaveCriticalSection(&g_WatchListCS);

	WatchDlg.DoModal();

	EnterCriticalSection(&g_WatchListCS);
	vector<CWatch*>::iterator it = find(g_WatchList.begin(), g_WatchList.end(), &WatchDlg);
	if (it != g_WatchList.end())
		g_WatchList.erase(it);

	for (it = g_WatchList.begin(); it != g_WatchList.end(); it++) {
		CWatch* pWatch = *it;
		if (pWatch->m_pAddress == lpParam)
			break;
	}
	if (it == g_WatchList.end())
		CheatEngine.debug_removeBreakpoint((UINT_PTR)lpParam);
	LeaveCriticalSection(&g_WatchListCS);
	return 0;
}

char buffer[0x100];
BOOL __stdcall OnDisassemblerPopup(UINT_PTR selectedAddress, char **addressofname, BOOL *show) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	UINT_PTR pAddress = *(UINT_PTR*)selectedAddress;

	REGISTERMODIFICATIONINFO info = { 0, };
	info.address = pAddress;

	if (CheatEngine.ChangeRegistersAtAddress(pAddress, &info))
		AfxBeginThread(DlgThread, (LPVOID)pAddress);

	return TRUE;
}

BOOL __stdcall OnDisassembler(ULONG *selectedAddress) {
	return TRUE;
}

BOOL __stdcall CEPlugin_InitializePlugin(PExportedFunctions ef, int pluginid)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	g_PluginID = pluginid;

	//copy the EF list to Exported
	CheatEngine = *ef; //Exported is defined in the .h
	if (CheatEngine.sizeofExportedFunctions != sizeof(CheatEngine))
		return FALSE;

	//On Debug event plugin	
	DEBUGEVENTPLUGIN_INIT  init2;
	init2.callbackroutine = debugeventplugin;
	g_DebugID = CheatEngine.RegisterFunction(pluginid, ptOnDebugEvent, &init2);
	if (g_DebugID == -1) {
		CheatEngine.ShowMessage("Failure to register the ondebugevent plugin");
		return FALSE;
	}

	//OnDisassembler plugin
	DISASSEMBLERCONTEXT_INIT init6;
	init6.name = "Watch expression";
	init6.callbackroutineOnPopup = OnDisassemblerPopup;
	init6.callbackroutine = OnDisassembler;
	init6.shortcut = "Ctrl+W";
	g_DisassemblerID = CheatEngine.RegisterFunction(pluginid, ptDisassemblerContext, &init6);
	if (g_DisassemblerID == -1) {
		CheatEngine.ShowMessage("Failure to register the ondisassembly plugin");
		return FALSE;
	}

	pCE_ReadProcessMemory = (TReadProcessMemory*)CheatEngine.ReadProcessMemory;
	pCE_WriteProcessMemory = (TWriteProcessMemory*)CheatEngine.WriteProcessMemory;
	pCE_OpenThread = (TOpenThread*)CheatEngine.OpenThread;

	//more compatible array
	size_t Result = AobscanModule(GetModuleHandle(0), "48 8b ? ? ? ? ? c6 ? ? 01 48 ? ? ? c6 ? ? 01");
	//48 83 3D ? ? ? ? 00 74 1C 48 ? ? 48
	if (Result == 0) {
		CheatEngine.ShowMessage("Not supported CE version.");
		return FALSE;
	}
	g_pCurrentDebuggerInterface = (size_t*)(Result + 0x7 + *(PDWORD)(Result + 0x3));

	InitializeCriticalSection(&g_WatchListCS);
	return TRUE;
}

BOOL __stdcall CEPlugin_GetVersion(PPluginVersion pv, int sizeofpluginversion)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pv->version = CESDK_VERSION;
	pv->pluginname = "Watch Expression Plugin(Available at CE 6.5+)";
	return TRUE;
}

BOOL __stdcall CEPlugin_DisablePlugin(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CheatEngine.UnregisterFunction(g_PluginID, g_DebugID);
	CheatEngine.UnregisterFunction(g_PluginID, g_DisassemblerID);

	//for (auto pWatch : g_WatchList)
	EnterCriticalSection(&g_WatchListCS);
	for (vector<CWatch*>::iterator it = g_WatchList.begin(); it != g_WatchList.end(); it++) {
		CWatch* pWatch = *it;
		pWatch->PostMessage(WM_CLOSE);
	}
	LeaveCriticalSection(&g_WatchListCS);

	while (1) {
		EnterCriticalSection(&g_WatchListCS);
		size_t Size = g_WatchList.size();
		LeaveCriticalSection(&g_WatchListCS);

		if (Size == 0)
			break;
		Sleep(1);
	}

	DeleteCriticalSection(&g_WatchListCS);
	return TRUE;
}
