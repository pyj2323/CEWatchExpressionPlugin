// Watch.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "Honoka.h"
#include "Watch.h"
#include "afxdialogex.h"


#define INDEX_COUNT 0
#define INDEX_VALUE 1



// CWatch 대화 상자입니다.

IMPLEMENT_DYNAMIC(CWatch, CDialog)

CWatch::CWatch(PVOID pAddress, CWnd* pParent /*=NULL*/)
	: CDialog(CWatch::IDD, pParent), m_pAddress(pAddress)
{
	m_nType = OUTPUT_INTEGER;
	m_nIntShowType = OUTPUT_DEC_SIGNED;
	m_nStrShowType = OUTPUT_ANSI;
	m_bCapture = FALSE;
}

BOOL CWatch::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetWindowRect(&m_DlgRect);
	m_listLog.GetWindowRect(&m_ListRect);

	BOOL bWow64 = FALSE;
	if(!IsWow64Process(*CheatEngine.OpenedProcessHandle, &bWow64)) {
		MessageBox(L"Open a process first.", 0, MB_ICONERROR);
		return FALSE;
	}

	CString szTitle;
	if(bWow64)
		szTitle.Format(L"Watch expression at address %08I64X", m_pAddress);
	else
		szTitle.Format(L"Watch expression at address %016I64X", m_pAddress);
	SetWindowText(szTitle);

	m_listLog.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	CRect rect;
	m_listLog.GetClientRect(&rect);

	m_listLog.InsertColumn(INDEX_COUNT, L"Count", LVCFMT_LEFT, 50);
	m_listLog.InsertColumn(INDEX_VALUE, L"Value", LVCFMT_LEFT, rect.Width() - 50);

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
	ON_BN_CLICKED(IDC_BUTTON_STATE, &CWatch::OnBnClickedButtonState)
	ON_MESSAGE(WM_GETCONTEXT, &CWatch::OnGetcontext)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &CWatch::OnBnClickedButtonClear)
	ON_BN_CLICKED(IDC_RADIO_OPT1, &CWatch::OnBnClickedRadioOpt1)
	ON_CBN_SELCHANGE(IDC_COMBO_TYPE, &CWatch::OnCbnSelchangeComboType)
	ON_BN_CLICKED(IDC_RADIO_OPT2, &CWatch::OnBnClickedRadioOpt2)
	ON_BN_CLICKED(IDC_RADIO_OPT3, &CWatch::OnBnClickedRadioOpt3)
//	ON_NOTIFY(HDN_ITEMDBLCLICK, 0, &CWatch::OnItemdblclickListLog)
ON_NOTIFY(NM_DBLCLK, IDC_LIST_LOG, &CWatch::OnDblclkListLog)
//ON_NOTIFY(HDN_ITEMDBLCLICK, 0, &CWatch::OnItemdblclickListLog)
ON_WM_SIZE()
ON_WM_SIZING()
ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


BOOL ParseInfo::bX86 = 0;

void CWatch::OnBnClickedButtonState()
{
	if(m_bCapture) {
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
	if(!IsWow64Process(*CheatEngine.OpenedProcessHandle, &bWow64)) {
		MessageBox(L"OpenProcess Failed.\n", L"Error", MB_ICONERROR);
		return;
	}

	ParseInfo::bX86 = bWow64;

	CStringW wExp;
	m_editExp.GetWindowText(wExp);
	m_szExp = CW2A(wExp, CP_ACP);

	CONTEXT ctx = {0};
	Parser parser(ctx);
	ParseInfo exp_info;

	if(!parser.Parse(m_szExp, exp_info)) {
		MessageBox(L"Parsing expression failed.", L"Error", MB_ICONERROR);
		return;
	}

	switch(m_nType) {
	case OUTPUT_INTEGER:
		break;
	case OUTPUT_STRING:
	case OUTPUT_AOB:
		{
			CStringW wLen;
			m_editLen.GetWindowText(wLen);
			m_szLen = CW2A(wLen, CP_ACP);

			ParseInfo len_info;
			if(!parser.Parse(m_szLen, len_info)) {
				MessageBox(L"Parsing data length failed.", L"Error", MB_ICONERROR);
				return;
			}
		}	
	case OUTPUT_OPCODE:
	case OUTPUT_FLOAT:
	case OUTPUT_DOUBLE:
		//Pointer type checking
		if(m_nType != OUTPUT_INTEGER) {
			if(bWow64 && exp_info.type != TYPE_DWORD) {
				MessageBox(L"4 bytes(DWORD) pointer required.", L"Error", MB_ICONERROR);
				return;
			}
			if(!bWow64 && exp_info.type != TYPE_QWORD) {
				MessageBox(L"8 bytes(QWORD) pointer required.", L"Error", MB_ICONERROR);
				return;
			}
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


afx_msg LRESULT CWatch::OnGetcontext(WPARAM wParam, LPARAM lParam)
{
	CString szValue = (const wchar_t*)wParam;
	delete (const wchar_t*)wParam;

	BOOL bExist = FALSE;

	int i;
	for(i = 0 ; i < m_listLog.GetItemCount() ; i++) {
		CString szItemValue = m_listLog.GetItemText(i, INDEX_VALUE);
		if(szValue == szItemValue) {
			bExist = TRUE;
			break;
		}
	}

	if(bExist) {
		//Add count
		CString szCount = m_listLog.GetItemText(i, INDEX_COUNT);
		UINT uCnt = _wtoi(szCount);
		szCount.Format(L"%d", uCnt + 1);
		m_listLog.SetItemText(i, INDEX_COUNT, szCount);
	}
	else {
		//Create new item
		int n = m_listLog.InsertItem(i, L"");
		m_listLog.SetItemText(n, INDEX_COUNT, L"1");
		m_listLog.SetItemText(n, INDEX_VALUE, szValue);
	}

	return 0;
}
void CWatch::OnBnClickedButtonClear()
{
	m_listLog.DeleteAllItems();
}
void CWatch::OnCbnSelchangeComboType()
{
	OutputType type = (OutputType)m_comboType.GetCurSel();

	switch(type) {
	case OUTPUT_INTEGER:
		//Hide length windows
		m_editLen.EnableWindow(FALSE);
		m_editLen.ShowWindow(SW_HIDE);

		m_staticLen.EnableWindow(FALSE);
		m_staticLen.ShowWindow(SW_HIDE);

		//Show radio buttons
		m_radioOpt1.EnableWindow(TRUE);
		m_radioOpt1.ShowWindow(SW_NORMAL);

		m_radioOpt2.EnableWindow(TRUE);
		m_radioOpt2.ShowWindow(SW_NORMAL);

		m_radioOpt3.EnableWindow(TRUE);
		m_radioOpt3.ShowWindow(SW_NORMAL);

		//Set radio buttons
		m_radioOpt1.SetWindowText(L"Dec signed");
		m_radioOpt2.SetWindowText(L"Dec unsigned");
		m_radioOpt3.SetWindowText(L"Hex");

		m_radioOpt1.SetCheck(m_nIntShowType == OUTPUT_DEC_SIGNED);
		m_radioOpt2.SetCheck(m_nIntShowType == OUTPUT_DEC_UNSIGNED);
		m_radioOpt3.SetCheck(m_nIntShowType == OUTPUT_HEX);
		break;
	case OUTPUT_OPCODE:
	case OUTPUT_FLOAT:
	case OUTPUT_DOUBLE:
		//Hide length windows
		m_editLen.EnableWindow(FALSE);
		m_editLen.ShowWindow(SW_HIDE);

		m_staticLen.EnableWindow(FALSE);
		m_staticLen.ShowWindow(SW_HIDE);

		//Hide radio buttons
		m_radioOpt1.EnableWindow(FALSE);
		m_radioOpt1.ShowWindow(SW_HIDE);

		m_radioOpt2.EnableWindow(FALSE);
		m_radioOpt2.ShowWindow(SW_HIDE);

		m_radioOpt3.EnableWindow(FALSE);
		m_radioOpt3.ShowWindow(SW_HIDE);
		break;
	case OUTPUT_STRING:
		//Show length windows
		m_editLen.EnableWindow(TRUE);
		m_editLen.ShowWindow(SW_NORMAL);

		m_staticLen.EnableWindow(TRUE);
		m_staticLen.ShowWindow(SW_NORMAL);

		//Show radio buttons
		m_radioOpt1.EnableWindow(TRUE);
		m_radioOpt1.ShowWindow(SW_NORMAL);

		m_radioOpt2.EnableWindow(TRUE);
		m_radioOpt2.ShowWindow(SW_NORMAL);

		m_radioOpt3.EnableWindow(TRUE);
		m_radioOpt3.ShowWindow(SW_NORMAL);

		//Set radio buttons
		m_radioOpt1.SetWindowText(L"ANSI");
		m_radioOpt2.SetWindowText(L"UTF-8");
		m_radioOpt3.SetWindowText(L"UTF-16");

		m_radioOpt1.SetCheck(m_nStrShowType == OUTPUT_ANSI);
		m_radioOpt2.SetCheck(m_nStrShowType == OUTPUT_UTF8);
		m_radioOpt3.SetCheck(m_nStrShowType == OUTPUT_UTF16);
		break;
	case OUTPUT_AOB:
		//Hide length windows
		m_editLen.EnableWindow(TRUE);
		m_editLen.ShowWindow(SW_NORMAL);

		m_staticLen.EnableWindow(TRUE);
		m_staticLen.ShowWindow(SW_NORMAL);

		//Hide radio buttons
		m_radioOpt1.EnableWindow(FALSE);
		m_radioOpt1.ShowWindow(SW_HIDE);

		m_radioOpt2.EnableWindow(FALSE);
		m_radioOpt2.ShowWindow(SW_HIDE);

		m_radioOpt3.EnableWindow(FALSE);
		m_radioOpt3.ShowWindow(SW_HIDE);
		break;
	}

	m_nType = type;
}
void CWatch::OnBnClickedRadioOpt1()
{
	switch(m_nType) {
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
	switch(m_nType) {
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
	switch(m_nType) {
	case OUTPUT_INTEGER:
		m_nIntShowType = OUTPUT_HEX;
		break;
	case OUTPUT_STRING:
		m_nStrShowType = OUTPUT_UTF16;
		break;
	}
}


VOID __stdcall SelectMemoryViewAddress(LPVOID parameters) {
	CStringA* szScript = (CStringA*)parameters;
	
	//This should open a memoryview window at specified address
	lua_State *lua = (lua_State*)CheatEngine.GetLuaState();
	if(lua) {
		int i = lua_gettop(lua);
		luaL_dostring(lua, *szScript);
		lua_settop(lua, i);	//restore
	}

	delete szScript;
}

void CWatch::OnDblclkListLog(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	POSITION pos = m_listLog.GetFirstSelectedItemPosition();
	if(pos == NULL)
		return;

	int nItem = m_listLog.GetNextSelectedItem(pos);
	CString szValue = m_listLog.GetItemText(nItem, INDEX_VALUE);

	int i = -1;

	i = szValue.Find(L" - ", 0);
	if(i == -1)
		return;

	CStringA* szScript = new CStringA();
	szScript->Format(		
		"local mv=getMemoryViewForm()\n"
		"mv.Disassemblerview.SelectedAddress = 0x%ls\n"
		"mv.show()\n", szValue.Left(i));

	CheatEngine.MainThreadCall(SelectMemoryViewAddress, szScript);
}

void CWatch::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	CRect DlgRect, ListRect;
	GetWindowRect(&DlgRect);
	m_listLog.GetWindowRect(&ListRect);

	int nAddX = DlgRect.Width() - m_DlgRect.Width();
	int nAddY = DlgRect.Height() - m_DlgRect.Height();

	int nListWidth = m_ListRect.Width() + nAddX;
	int nListHeight = m_ListRect.Height() + nAddY;

	m_listLog.SetWindowPos(NULL, 0, 0, nListWidth, nListHeight, SWP_NOMOVE);

	int nColumnWidth = m_listLog.GetColumnWidth(INDEX_VALUE) + nListWidth - ListRect.Width();
	m_listLog.SetColumnWidth(INDEX_VALUE, nColumnWidth);
}


void CWatch::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);

	CRect DlgRect, ListRect;
	GetWindowRect(&DlgRect);
	m_listLog.GetWindowRect(&ListRect);

	int nAddX = DlgRect.Width() - m_DlgRect.Width();
	int nAddY = DlgRect.Height() - m_DlgRect.Height();

	int nListWidth = m_ListRect.Width() + nAddX;
	int nListHeight = m_ListRect.Height() + nAddY;

	m_listLog.SetWindowPos(NULL, 0, 0, nListWidth, nListHeight, SWP_NOMOVE);

	int nColumnWidth = m_listLog.GetColumnWidth(INDEX_VALUE) + nListWidth - ListRect.Width();
	m_listLog.SetColumnWidth(INDEX_VALUE, nColumnWidth);
}


void CWatch::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	lpMMI->ptMinTrackSize.x = m_DlgRect.Width();
	lpMMI->ptMinTrackSize.y = m_DlgRect.Height();
	CDialog::OnGetMinMaxInfo(lpMMI);
}
