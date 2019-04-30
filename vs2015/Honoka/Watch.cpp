// Watch.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "Honoka.h"
#include "Watch.h"
//#include "afxdialogex.h"


//extern "C" BOOL IsWow64Process(HANDLE hProcess, PBOOL Wow64Process);
//#define LVS_EX_DOUBLEBUFFER     0x00010000

// CWatch 대화 상자입니다.

IMPLEMENT_DYNAMIC(CWatch, CDialog)

CWatch::CWatch(PVOID pAddress, CWnd* pParent /*=NULL*/)
	: CDialog(CWatch::IDD, pParent), m_pAddress(pAddress)
{
	m_nType = OUTPUT_INTEGER;
	m_nIntShowType = OUTPUT_DEC_SIGNED;
	m_nStrShowType = OUTPUT_ANSI;
	m_bCapture = FALSE;
	m_bRect = FALSE;
}

BOOL CWatch::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_hAccel = ::LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR2));

	GetWindowRect(&m_DlgRect);
	m_listLog.GetWindowRect(&m_ListRect);
	m_bRect = TRUE;

	BOOL bWow64 = FALSE;
	if (!IsWow64Process(*CheatEngine.OpenedProcessHandle, &bWow64)) {
		MessageBox(L"Open a process first.", 0, MB_ICONERROR);
		return FALSE;
	}

	CString szTitle;
	if (bWow64)
		szTitle.Format(L"Watch expression at address %08I64X", m_pAddress);
	else
		szTitle.Format(L"Watch expression at address %016I64X", m_pAddress);
	SetWindowText(szTitle);

	CRect rect;
	m_listLog.GetClientRect(&rect);

	m_listLog.InsertColumn(INDEX_COUNT, L"Count", LVCFMT_LEFT, 50);
	m_listLog.InsertColumn(INDEX_VALUE, L"Value", LVCFMT_LEFT, rect.Width() - 50);

	m_listLog.SetExtendedStyle(m_listLog.GetExtendedStyle() | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);

	m_comboType.InsertString(OUTPUT_INTEGER, L"Integer");
	m_comboType.InsertString(OUTPUT_OPCODE, L"Opcode");
	m_comboType.InsertString(OUTPUT_FLOAT, L"Float");
	m_comboType.InsertString(OUTPUT_DOUBLE, L"Double");
	m_comboType.InsertString(OUTPUT_STRING, L"String");
	m_comboType.InsertString(OUTPUT_AOB, L"Array of bytes");

	//Set default length
	m_editLen.SetWindowText(L"10");

	//Set default type
	m_comboType.SetCurSel(OUTPUT_INTEGER);

	OnCbnSelchangeComboType();
	return TRUE;
}


CWatch::~CWatch()
{
}

void CWatch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_STATE, m_btnState);
	DDX_Control(pDX, IDC_LIST_LOG, m_listLog);
	DDX_Control(pDX, IDC_EDIT_EXPRESSION, m_editExp);
	DDX_Control(pDX, IDC_COMBO_TYPE, m_comboType);
	DDX_Control(pDX, IDC_EDIT_LENGTH, m_editLen);
	DDX_Control(pDX, IDC_RADIO_OPT1, m_radioOpt1);
	DDX_Control(pDX, IDC_RADIO_OPT2, m_radioOpt2);
	DDX_Control(pDX, IDC_RADIO_OPT3, m_radioOpt3);
	DDX_Control(pDX, IDC_STATIC_LENGTH, m_staticLen);
}

BEGIN_MESSAGE_MAP(CWatch, CDialog)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_STATE, CWatch::OnBnClickedButtonState)
	//	ON_MESSAGE(WM_GETCONTEXT, CWatch::OnGetcontext)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, CWatch::OnBnClickedButtonClear)
	ON_BN_CLICKED(IDC_RADIO_OPT1, CWatch::OnBnClickedRadioOpt1)
	ON_CBN_SELCHANGE(IDC_COMBO_TYPE, CWatch::OnCbnSelchangeComboType)
	ON_BN_CLICKED(IDC_RADIO_OPT2, CWatch::OnBnClickedRadioOpt2)
	ON_BN_CLICKED(IDC_RADIO_OPT3, CWatch::OnBnClickedRadioOpt3)
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_GETMINMAXINFO()
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_ACCELERATOR_CTRL_T, CWatch::OnAcceleratorCtrlT)
END_MESSAGE_MAP()


BOOL ParseInfo::bX86 = 0;

void CWatch::OnBnClickedButtonState()
{
	if (m_bCapture) {
		m_btnState.SetWindowText(L"Start");
		m_editExp.EnableWindow(TRUE);
		m_comboType.EnableWindow(TRUE);
		m_editLen.EnableWindow(TRUE);
		m_radioOpt1.EnableWindow(TRUE);
		m_radioOpt2.EnableWindow(TRUE);
		m_radioOpt3.EnableWindow(TRUE);
		m_bCapture = FALSE;
		return;
	}

	BOOL bWow64;
	if (!IsWow64Process(*CheatEngine.OpenedProcessHandle, &bWow64)) {
		MessageBox(L"OpenProcess Failed.", L"Error", MB_ICONERROR);
		return;
	}

	ParseInfo::bX86 = bWow64;

	m_editExp.GetWindowText(m_szExp);

	CONTEXT ctx = { 0 };
	Parser parser(ctx);
	ParseInfo exp_info = parser.Parse(m_szExp);

	CString TypeResult;
	switch (exp_info.Type) {
	case TYPE_BIT:		TypeResult = "Result is BIT type.\n"; break;
	case TYPE_BYTE:		TypeResult = "Result is BYTE type.\n"; break;
	case TYPE_WORD:		TypeResult = "Result is WORD type.\n"; break;
	case TYPE_DWORD:	TypeResult = "Result is DWORD type.\n"; break;
	case TYPE_QWORD:	TypeResult = "Result is QWORD type.\n"; break;
	case TYPE_FLOAT:	TypeResult = "Result is FLOAT type.\n"; break;
	case TYPE_DOUBLE:	TypeResult = "Result is DOUBLE type.\n"; break;
	}

	if (exp_info.Error & ERROR_PARSE || exp_info.Error & ERROR_TYPE) {
		MessageBox(L"Parsing failed.", L"Error", MB_ICONERROR);
		return;
	}

	switch (m_nType) {
	case OUTPUT_INTEGER:
		if (exp_info.IsFloating()) {
			MessageBox(TypeResult + L"Integer data required.", L"Error", MB_ICONERROR);
			return;
		}
		break;
	case OUTPUT_FLOAT:
		if (exp_info.Type != TYPE_FLOAT) {
			MessageBox(TypeResult + L"Float data required.", L"Error", MB_ICONERROR);
			return;
		}
		break;
	case OUTPUT_DOUBLE:
		if (exp_info.Type != TYPE_DOUBLE) {
			MessageBox(TypeResult + L"Double data required.", L"Error", MB_ICONERROR);
			return;
		}
		break;
	case OUTPUT_STRING:
	case OUTPUT_AOB:
	{
		m_editLen.GetWindowText(m_szLen);

		ParseInfo len_info = parser.Parse(m_szLen);

		if (len_info.Error & ERROR_PARSE || len_info.Error & ERROR_TYPE || len_info.IsFloating()) {
			MessageBox(L"Parsing length failed.", L"Error", MB_ICONERROR);
			return;
		}
	}
	case OUTPUT_OPCODE:
		//Pointer type checking
		if (bWow64 && exp_info.Type != TYPE_DWORD) {
			MessageBox(TypeResult + L"4 bytes(DWORD) pointer required.", L"Error", MB_ICONERROR);
			return;
		}
		if (!bWow64 && exp_info.Type != TYPE_QWORD) {
			MessageBox(TypeResult + L"8 bytes(QWORD) pointer required.", L"Error", MB_ICONERROR);
			return;
		}
		break;
	}

	m_btnState.SetWindowText(L"Stop");
	m_editExp.EnableWindow(FALSE);
	m_comboType.EnableWindow(FALSE);
	m_editLen.EnableWindow(FALSE);
	m_radioOpt1.EnableWindow(FALSE);
	m_radioOpt2.EnableWindow(FALSE);
	m_radioOpt3.EnableWindow(FALSE);
	m_bCapture = TRUE;
}

void CWatch::AddItem(CONTEXT& ctx) {
	Parser parser(ctx);
	ParseInfo exp_info = parser.Parse(m_szExp);

	CString szValue;
	char szBuffer[0x201];
	WCHAR wBuffer[0x201];

	if (exp_info.Error & ERROR_PARSE)
		return;

	if (exp_info.Error & ERROR_PAGEFAULT)
		szValue = L"Error : Access violation";
	else if (exp_info.Error & ERROR_DEVIDEZERO)
		szValue = L"Error : Divide by zero";
	else if (exp_info.Error == 0) {
		switch (m_nType) {
		case OUTPUT_INTEGER:
			switch (m_nIntShowType) {
			case OUTPUT_DEC_SIGNED:
				szValue.Format(L"%I64d", exp_info.s64Value);
				break;
			case OUTPUT_DEC_UNSIGNED:
				szValue.Format(L"%I64u", exp_info.u64Value);
				break;
			case OUTPUT_HEX:
				switch (exp_info.Type) {
				case TYPE_BIT:
					szValue = exp_info.bValue ? L"true" : L"false";
					break;
				case TYPE_BYTE:
					szValue.Format(L"0x%02I64X", exp_info.u8Value);
					break;
				case TYPE_WORD:
					szValue.Format(L"0x%04I64X", exp_info.u16Value);
					break;
				case TYPE_DWORD:
					szValue.Format(L"0x%08I64X", exp_info.u32Value);
					break;
				case TYPE_QWORD:
					szValue.Format(L"0x%016I64X", exp_info.u64Value);
					break;
				}
				break;
			}
			break;
		case OUTPUT_OPCODE:
		{
			if (CheatEngine.Disassembler(exp_info.u64Value, szBuffer, sizeof(szBuffer))) {
				MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, wBuffer, sizeof(wBuffer));
				szValue = wBuffer;
			}
			else
				szValue = L"Error : Access violation";
			break;
		}
		case OUTPUT_FLOAT:
			szValue.Format(L"%f", exp_info.fValue);
			break;
		case OUTPUT_DOUBLE:
			szValue.Format(L"%f", exp_info.dValue);
			break;
			break;
		case OUTPUT_STRING:
		{
			ParseInfo len_info = parser.Parse(m_szLen);
			if (len_info.Error & ERROR_PARSE)
				break;
			if (len_info.Error & ERROR_PAGEFAULT) {
				szValue = L"Error : Access violation";
				break;
			}
			if (len_info.Error & ERROR_DEVIDEZERO) {
				szValue = L"Error : Divide by zero";
				break;
			}

			SIZE_T nRead, nLen = len_info.u64Value;
			if (nLen >= 0x200) {
				szValue = L"Error : Length must be less than 512.";
				break;
			}
			if (nLen == 0) {
				szValue = L"Error : Length is 0.";
				break;
			}

			switch (m_nStrShowType) {
			case OUTPUT_ANSI:
			{
				BOOL bResult = CE_ReadProcessMemory(
					*CheatEngine.OpenedProcessHandle,
					(LPCVOID)exp_info.u64Value, szBuffer, nLen, &nRead);

				szBuffer[nLen] = 0;

				if (bResult && nRead == nLen) {
					MultiByteToWideChar(CP_ACP, 0, szBuffer, -1, wBuffer, sizeof(wBuffer));
					szValue.Format(L"\"%ws\"", wBuffer);
				}
				else
					szValue = L"Error : Access violation";

				break;
			}
			case OUTPUT_UTF8:
			{
				BOOL bResult = CE_ReadProcessMemory(
					*CheatEngine.OpenedProcessHandle,
					(LPCVOID)exp_info.u64Value, szBuffer, nLen, &nRead);

				szBuffer[nLen] = 0;

				if (bResult && nRead == nLen) {
					MultiByteToWideChar(CP_UTF8, 0, szBuffer, -1, wBuffer, sizeof(wBuffer));
					szValue.Format(L"\"%ws\"", wBuffer);
				}
				else
					szValue = L"Error : Access violation";

				break;
			}
			case OUTPUT_UTF16:
			{
				BOOL bResult = CE_ReadProcessMemory(
					*CheatEngine.OpenedProcessHandle,
					(LPCVOID)exp_info.u64Value, wBuffer, nLen * sizeof(WCHAR), &nRead);

				wBuffer[nLen] = 0;

				if (bResult && nRead == nLen * sizeof(WCHAR))
					szValue.Format(L"\"%ws\"", wBuffer);
				else
					szValue = L"Error : Access violation";

				break;
			}
			}
			break;
		}
		case OUTPUT_AOB:
		{
			ParseInfo len_info = parser.Parse(m_szLen);

			if (len_info.Error & ERROR_PARSE)
				break;
			if (len_info.Error & ERROR_PAGEFAULT) {
				szValue = L"Error : Access violation";
				break;
			}
			if (len_info.Error & ERROR_DEVIDEZERO) {
				szValue = L"Error : Divide by zero";
				break;
			}

			SIZE_T nRead, nLen = len_info.u64Value;

			if (nLen >= 150) {
				szValue = L"Error : Length must be less than 150.";
				break;
			}
			if (nLen == 0) {
				szValue = L"Error : Length is 0.";
				break;
			}

			BOOL bResult = CE_ReadProcessMemory(
				*CheatEngine.OpenedProcessHandle,
				(LPCVOID)exp_info.u64Value, szBuffer, nLen, &nRead);

			if (bResult && nRead == nLen) {
				CString szByte;
				for (int i = 0; i < nLen; i++) {
					szByte.Format(L"%02X", (BYTE)szBuffer[i]);
					szValue += szByte;
					if (i < nLen - 1)
						szValue += L" ";
				}
			}
			else
				szValue = L"Error : Access violation";

			break;
		}
		}
	}

	m_listLog.AddItem(szValue);
}

void CWatch::OnBnClickedButtonClear() {
	m_listLog.Clear();
}

void CWatch::OnCbnSelchangeComboType()
{
	OutputType type = (OutputType)m_comboType.GetCurSel();

	//Disable ALL
	m_editLen.EnableWindow(FALSE);
	m_editLen.ShowWindow(SW_HIDE);

	m_staticLen.EnableWindow(FALSE);
	m_staticLen.ShowWindow(SW_HIDE);

	m_radioOpt1.EnableWindow(FALSE);
	m_radioOpt2.EnableWindow(FALSE);
	m_radioOpt3.EnableWindow(FALSE);
	m_radioOpt1.ShowWindow(SW_HIDE);
	m_radioOpt2.ShowWindow(SW_HIDE);
	m_radioOpt3.ShowWindow(SW_HIDE);

	switch (type) {
	case OUTPUT_INTEGER:
		m_radioOpt1.EnableWindow(TRUE);
		m_radioOpt2.EnableWindow(TRUE);
		m_radioOpt3.EnableWindow(TRUE);

		m_radioOpt1.SetWindowText(L"Dec signed");
		m_radioOpt2.SetWindowText(L"Dec unsigned");
		m_radioOpt3.SetWindowText(L"Hex");
		m_radioOpt1.SetCheck(m_nIntShowType == OUTPUT_DEC_SIGNED);
		m_radioOpt2.SetCheck(m_nIntShowType == OUTPUT_DEC_UNSIGNED);
		m_radioOpt3.SetCheck(m_nIntShowType == OUTPUT_HEX);

		m_radioOpt1.ShowWindow(SW_NORMAL);
		m_radioOpt2.ShowWindow(SW_NORMAL);
		m_radioOpt3.ShowWindow(SW_NORMAL);
		break;
	case OUTPUT_OPCODE:
		break;
	case OUTPUT_FLOAT:
	case OUTPUT_DOUBLE:
		break;
	case OUTPUT_STRING:
		m_editLen.EnableWindow(TRUE);
		m_staticLen.EnableWindow(TRUE);

		m_radioOpt1.EnableWindow(TRUE);
		m_radioOpt2.EnableWindow(TRUE);
		m_radioOpt3.EnableWindow(TRUE);

		m_radioOpt1.SetWindowText(L"ANSI");
		m_radioOpt2.SetWindowText(L"UTF-8");
		m_radioOpt3.SetWindowText(L"UTF-16");
		m_radioOpt1.SetCheck(m_nStrShowType == OUTPUT_ANSI);
		m_radioOpt2.SetCheck(m_nStrShowType == OUTPUT_UTF8);
		m_radioOpt3.SetCheck(m_nStrShowType == OUTPUT_UTF16);

		m_editLen.ShowWindow(SW_NORMAL);
		m_staticLen.ShowWindow(SW_NORMAL);

		m_radioOpt1.ShowWindow(SW_NORMAL);
		m_radioOpt2.ShowWindow(SW_NORMAL);
		m_radioOpt3.ShowWindow(SW_NORMAL);
		break;
	case OUTPUT_AOB:
		m_editLen.EnableWindow(TRUE);
		m_staticLen.EnableWindow(TRUE);

		m_editLen.ShowWindow(SW_NORMAL);
		m_staticLen.ShowWindow(SW_NORMAL);
		break;
	}

	m_nType = type;
}

void CWatch::OnBnClickedRadioOpt1()
{
	switch (m_nType) {
	case OUTPUT_INTEGER:
		m_nIntShowType = OUTPUT_DEC_SIGNED;
		break;
	case OUTPUT_STRING:
		m_nStrShowType = OUTPUT_ANSI;
		break;
	}
}

void CWatch::OnBnClickedRadioOpt2()
{
	switch (m_nType) {
	case OUTPUT_INTEGER:
		m_nIntShowType = OUTPUT_DEC_UNSIGNED;
		break;
	case OUTPUT_STRING:
		m_nStrShowType = OUTPUT_UTF8;
		break;
	}
}

void CWatch::OnBnClickedRadioOpt3()
{
	switch (m_nType) {
	case OUTPUT_INTEGER:
		m_nIntShowType = OUTPUT_HEX;
		break;
	case OUTPUT_STRING:
		m_nStrShowType = OUTPUT_UTF16;
		break;
	}
}

void CWatch::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if (!m_bRect)
		return;

	CRect DlgRect, ListRect;
	GetWindowRect(&DlgRect);
	m_listLog.GetWindowRect(&ListRect);

	int nAddX = DlgRect.Width() - m_DlgRect.Width();
	int nAddY = DlgRect.Height() - m_DlgRect.Height();

	int nListWidth = m_ListRect.Width() + nAddX;
	int nListHeight = m_ListRect.Height() + nAddY;

	m_listLog.SetWindowPos(NULL, 0, 0, nListWidth, nListHeight, SWP_NOMOVE);

	int nColumnWidth = nListWidth - m_listLog.GetColumnWidth(INDEX_COUNT) - 1;
	m_listLog.SetColumnWidth(INDEX_VALUE, nColumnWidth);
}

void CWatch::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);
	if (!m_bRect)
		return;

	CRect DlgRect, ListRect;
	GetWindowRect(&DlgRect);
	m_listLog.GetWindowRect(&ListRect);

	int nAddX = DlgRect.Width() - m_DlgRect.Width();
	int nAddY = DlgRect.Height() - m_DlgRect.Height();

	int nListWidth = m_ListRect.Width() + nAddX;
	int nListHeight = m_ListRect.Height() + nAddY;

	m_listLog.SetWindowPos(NULL, 0, 0, nListWidth, nListHeight, SWP_NOMOVE);

	int nColumnWidth = nListWidth - m_listLog.GetColumnWidth(INDEX_COUNT) - 1;
	m_listLog.SetColumnWidth(INDEX_VALUE, nColumnWidth);
}

void CWatch::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	CDialog::OnGetMinMaxInfo(lpMMI);
	if (!m_bRect)
		return;

	lpMMI->ptMinTrackSize.x = m_DlgRect.Width();
	if (m_listLog.m_bSingleMode) {
		lpMMI->ptMinTrackSize.y = 210;
		lpMMI->ptMaxTrackSize.y = 210;
	}
	else {
		lpMMI->ptMinTrackSize.y = m_DlgRect.Height();
	}
}

void CWatch::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_listLog.HandleRClick();
	CDialog::OnRButtonDown(nFlags, point);
}

void CWatch::OnAcceleratorCtrlT() {
	m_listLog.ToggleMode();
}

BOOL CWatch::PreTranslateMessage(MSG* pMsg)
{
	if (::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
		return TRUE;

	return CDialog::PreTranslateMessage(pMsg);
}