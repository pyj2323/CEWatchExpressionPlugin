// Honoka.cpp : 해당 DLL의 초기화 루틴을 정의합니다.
#include "stdafx.h"
#include "Honoka.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//TODO: 이 DLL이 MFC DLL에 대해 동적으로 링크되어 있는 경우
//		MFC로 호출되는 이 DLL에서 내보내지는 모든 함수의
//		시작 부분에 AFX_MANAGE_STATE 매크로가
//		들어 있어야 합니다.
//
//		예:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// 일반적인 함수 본문은 여기에 옵니다.
//		}


// CHonokaApp
BEGIN_MESSAGE_MAP(CHonokaApp, CWinApp)
END_MESSAGE_MAP()

// CHonokaApp 생성
CHonokaApp::CHonokaApp(){}
CHonokaApp theApp;
BOOL CHonokaApp::InitInstance()
{
	CWinApp::InitInstance();
	return TRUE;
}


int __stdcall debugeventplugin(LPDEBUG_EVENT lpDebugEvent)
{
	DWORD dwDebugEventCode = lpDebugEvent->dwDebugEventCode;

	if(dwDebugEventCode != EXCEPTION_DEBUG_EVENT) 
		return 0;

	DWORD dwProcessId = lpDebugEvent->dwProcessId;
	DWORD dwThreadId = lpDebugEvent->dwThreadId;

	HANDLE hThread = CE_OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadId);
	if(!hThread)
		return 0;

	CONTEXT ctx = {0};
	ctx.ContextFlags = CONTEXT_ALL;

	BOOL bResult = CE_GetThreadContext(hThread, &ctx, TRUE);

	//Set resume flag
	//EFlags_SetRF(ctx.EFlags, TRUE);
	//SetThreadContext(hThread, &ctx);
	//ContinueDebugEvent(dwProcessId, dwThreadId, DBG_CONTINUE);

	CloseHandle(hThread);

	if(!bResult)
		return 0;

	PVOID pExceptionAddress;

	EXCEPTION_BREAKPOINT;

	switch(lpDebugEvent->u.Exception.ExceptionRecord.ExceptionCode) {
	case STATUS_SINGLE_STEP:		//Hardware BP
	case 0x4000001E:				//windows debugger on x86 process
		pExceptionAddress = (PVOID)(ctx.Rip);
		break;
	case STATUS_BREAKPOINT:			//Software BP
	case 0x4000001F:				//windows debugger on x86 process
		pExceptionAddress = (PVOID)(ctx.Rip - 1);
		break;
	case STATUS_ACCESS_VIOLATION:	//Page Exception BP
		pExceptionAddress = (PVOID)lpDebugEvent->u.Exception.ExceptionRecord.ExceptionInformation[1];
		if(pExceptionAddress != (PVOID)ctx.Rip) 
			return 0;
		break;
	default:
		return 0;
	}

	for(auto pWatch : g_WatchList) {
		CWatch& WatchDlg = *pWatch;
		if(WatchDlg.m_pAddress != pExceptionAddress)
			continue;
		if(!WatchDlg.m_bCapture)
			continue;

		Parser parser(ctx);
		ParseInfo exp_info;
		if(!parser.Parse(WatchDlg.m_szExp, exp_info))
			continue;

		CStringW szValue;

		if(!exp_info.bValidPtr)
			szValue = L"Error : read access violation";
		else {
			switch(WatchDlg.m_nType) {
			case OUTPUT_INTEGER:
				switch(WatchDlg.m_nIntShowType) {
				case OUTPUT_DEC_SIGNED:
					szValue.Format(L"%I64d", exp_info.value);
					break;
				case OUTPUT_DEC_UNSIGNED:
					szValue.Format(L"%I64u", exp_info.value);
					break;
				case OUTPUT_HEX:
					switch(exp_info.type) {
					case TYPE_BOOL:
						szValue.Format(L"%01I64X", exp_info.value);
						break;
					case TYPE_BYTE:
						szValue.Format(L"%02I64X", exp_info.value);
						break;
					case TYPE_WORD:
						szValue.Format(L"%04I64X", exp_info.value);
						break;
					case TYPE_DWORD:
						szValue.Format(L"%08I64X", exp_info.value);
						break;
					case TYPE_QWORD:
						szValue.Format(L"%016I64X", exp_info.value);
						break;
					}
					break;
				}
				break;
			case OUTPUT_OPCODE:
			{
				char szOutput[0x100];
				if (CheatEngine.Disassembler(exp_info.value, szOutput, sizeof(szOutput)))
					szValue = CA2W(szOutput, CP_ACP);
				else
					szValue = L"Error : read access violation";
				break;
			}
			case OUTPUT_FLOAT:
				{
					float fValue;
					SIZE_T nRead;
					BOOL bResult = CE_ReadProcessMemory(
						*CheatEngine.OpenedProcessHandle, 
						(LPCVOID)exp_info.value, &fValue, sizeof(float), &nRead);
					if(bResult && nRead == sizeof(float))
						szValue.Format(L"%f", fValue);
					else
						szValue = L"Error : read access violation";
				}
				break;
			case OUTPUT_DOUBLE:
				{
					double dValue;
					SIZE_T nRead;
					BOOL bResult = CE_ReadProcessMemory(
						*CheatEngine.OpenedProcessHandle, 
						(LPCVOID)exp_info.value, &dValue, sizeof(double), &nRead);
					if(bResult && nRead == sizeof(double))
						szValue.Format(L"%f", dValue);
					else
						szValue = L"Error : read access violation";
				}
				break;
			case OUTPUT_STRING:
				{
					ParseInfo len_info;
					if(!parser.Parse(WatchDlg.m_szLen, len_info))
						break;
					if(!len_info.bValidPtr || len_info.value >= 0x200) {
						szValue = L"Error : length must be less than 512.";
						break;
					}
					if(len_info.value == 0) {
						szValue = L"Error : length is 0.";
						break;
					}

					SIZE_T nRead;

					switch(WatchDlg.m_nStrShowType) {
					case OUTPUT_ANSI:
						{
							SIZE_T nLen = len_info.value * sizeof(char);
							char* szBuf = new char[nLen + 1];
							szBuf[nLen] = 0;

							BOOL bResult = CE_ReadProcessMemory(
								*CheatEngine.OpenedProcessHandle,
								(LPCVOID)exp_info.value, szBuf, nLen, &nRead);
							if(bResult && nRead == nLen)
								szValue = CString(L"\"") + CA2W(szBuf, CP_ACP) + L"\"";
							else
								szValue = L"Error : read access violation";

							delete[] szBuf;
							break;
						}

					case OUTPUT_UTF8:
						{
							SIZE_T nLen = len_info.value * sizeof(char);
							char* szBuf = new char[nLen + 1];
							szBuf[nLen] = 0;

							BOOL bResult = CE_ReadProcessMemory(
								*CheatEngine.OpenedProcessHandle,
								(LPCVOID)exp_info.value, szBuf, nLen, &nRead);
							if(bResult && nRead == nLen)
								szValue = CString(L"\"") + CA2W(szBuf, CP_UTF8) + L"\"";
							else
								szValue = L"Error : read access violation";

							delete[] szBuf;
							break;
						}
					case OUTPUT_UTF16:
						{
							SIZE_T nLen =len_info.value * sizeof(wchar_t);
							wchar_t* wBuf = new wchar_t[nLen + 1];
							wBuf[nLen] = 0;

							BOOL bResult = CE_ReadProcessMemory(
								*CheatEngine.OpenedProcessHandle,
								(LPCVOID)exp_info.value, wBuf, nLen, &nRead);
							if(bResult && nRead == nLen)
								szValue = CString(L"\"") + wBuf + L"\"";
							else
								szValue = L"Error : read access violation";

							delete[] wBuf;
							break;
						}
					}
				}
				break;
			case OUTPUT_AOB:
				{
					ParseInfo len_info;
					if(!parser.Parse(WatchDlg.m_szLen, len_info))
						break;
					if(!len_info.bValidPtr || len_info.value >= 0x200) {
						szValue = L"Error : length must be less than 512.";
						break;
					}
					if(len_info.value == 0) {
						szValue = L"Error : length is 0.";
						break;
					}

					SIZE_T nLen = len_info.value;
					BYTE* pBuf = new BYTE[nLen];
					SIZE_T nRead;

					BOOL bResult = CE_ReadProcessMemory(
						*CheatEngine.OpenedProcessHandle,
						(LPCVOID)exp_info.value, pBuf, nLen, &nRead);
					if(bResult && nRead == nLen) {
						CString szByte;
						for(int i = 0 ; i < nLen ; i++) {
							szByte.Format(L"%02X", pBuf[i]);
							szValue += szByte;
							if(i < nLen - 1)
								szValue += L" ";
						}
					}
					else
						szValue = L"Error : read access violation";

					delete[] pBuf;
					break;
				}
			}
		}

		wchar_t* pStr = new wchar_t[szValue.GetLength() + 1];
		wcscpy(pStr, szValue);

		if(!WatchDlg.PostMessage(WM_GETCONTEXT, (WPARAM)pStr))
			delete pStr;
	}
	return 0; 
}

UINT DlgThread(LPVOID lpParam) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWatch WatchDlg(lpParam);

	g_WatchList.push_back(&WatchDlg);

	WatchDlg.DoModal();
	
	auto pos = find(g_WatchList.begin(), g_WatchList.end(), &WatchDlg);
	if(pos != g_WatchList.end())
		g_WatchList.erase(pos);

	BOOL bDeleteBP = TRUE;
	for (auto pWatch : g_WatchList)
		if(pWatch->m_pAddress == lpParam)
			bDeleteBP = FALSE;

	if(bDeleteBP)
		CheatEngine.debug_removeBreakpoint((UINT_PTR)lpParam);

	return 0;
}

char buffer[0x100];
BOOL __stdcall OnDisassemblerPopup(UINT_PTR selectedAddress, char **addressofname, BOOL *show) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	UINT_PTR pAddress = *(UINT_PTR*)selectedAddress;

	REGISTERMODIFICATIONINFO info = {0,};
	info.address = pAddress;

	if(CheatEngine.ChangeRegistersAtAddress(pAddress, &info))
		AfxBeginThread(DlgThread, (LPVOID)pAddress);

	return TRUE;
}

BOOL __stdcall OnDisassembler(ULONG *selectedAddress) {
	return TRUE;
}

BOOL __stdcall CEPlugin_InitializePlugin(PExportedFunctions ef , int pluginid)
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
	if(g_DebugID == -1) {
		CheatEngine.ShowMessage("Failure to register the ondebugevent plugin");
		return FALSE;
	}

	//OnDisassembler plugin
	DISASSEMBLERCONTEXT_INIT init6;
	init6.name = "Watch expression";
	init6.callbackroutineOnPopup = OnDisassemblerPopup;
	init6.callbackroutine = OnDisassembler;
	init6.shortcut="Ctrl+W";
	g_DisassemblerID = CheatEngine.RegisterFunction(pluginid, ptDisassemblerContext, &init6);
	if(g_DisassemblerID == -1) {
		CheatEngine.ShowMessage("Failure to register the ondisassembly plugin");
		return FALSE;
	}

	pCE_ReadProcessMemory = (TReadProcessMemory*)CheatEngine.ReadProcessMemory;
	pCE_WriteProcessMemory = (TWriteProcessMemory*)CheatEngine.WriteProcessMemory;
	pCE_OpenThread = (TOpenThread*)CheatEngine.OpenThread;

	/*
	size_t Result = AobscanModule(GetModuleHandle(0), "48 83 3D ? ? ? ? 00 74 1C 48 ? ? 48");
	if(Result == 0) {
		CheatEngine.ShowMessage("Not supported CE version.");
		return FALSE;
	}
	g_pCurrentDebuggerInterface = (size_t*)(Result + 0x8 + *(PDWORD)(Result + 0x3));
	*/

	//more compatible array
	size_t Result = AobscanModule(GetModuleHandle(0), "48 8b ? ? ? ? ? c6 ? ? 01 48 ? ? ? c6 ? ? 01");
	if(Result == 0) {
		CheatEngine.ShowMessage("Not supported CE version.");
		return FALSE;
	}
	g_pCurrentDebuggerInterface = (size_t*)(Result + 0x7 + *(PDWORD)(Result + 0x3));
	return TRUE;
}

BOOL __stdcall CEPlugin_GetVersion(PPluginVersion pv , int sizeofpluginversion)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pv->version=CESDK_VERSION;
	pv->pluginname="Watch Expression Plugin(Available at CE 6.5+)";
	return TRUE;
}

BOOL __stdcall CEPlugin_DisablePlugin(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	for (auto pWatch : g_WatchList)
		pWatch->PostMessage(WM_CLOSE);

	while(g_WatchList.size())
		Sleep(1);

	return TRUE;
}