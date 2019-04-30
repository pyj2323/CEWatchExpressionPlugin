#include "stdafx.h"
#include "MyListCtrl.h"


CMyListCtrl::CMyListCtrl() {
	m_bSingleMode = FALSE;
}

CMyListCtrl::~CMyListCtrl() {}

BEGIN_MESSAGE_MAP(CMyListCtrl, CListCtrl)
	ON_WM_NCCALCSIZE()
	ON_COMMAND(ID_ACCELERATOR_CTRL_A, CMyListCtrl::OnAcceleratorCtrlA)
	ON_COMMAND(ID_ACCELERATOR_CTRL_C, CMyListCtrl::OnAcceleratorCtrlC)
	ON_COMMAND(ID_ACCELERATOR_CTRL_G, CMyListCtrl::OnAcceleratorCtrlG)
	ON_COMMAND(ID_ACCELERATOR_CTRL_R, CMyListCtrl::OnAcceleratorCtrlR)
	ON_NOTIFY_REFLECT(NM_DBLCLK, CMyListCtrl::OnNMDblclk)
	ON_COMMAND(ID_ACCELERATOR_CTRL_T, CMyListCtrl::OnAcceleratorCtrlT)
	ON_NOTIFY_REFLECT(NM_RCLICK, CMyListCtrl::OnNMRClick)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, CMyListCtrl::OnLvnKeydown)
END_MESSAGE_MAP()


void CMyListCtrl::PreSubclassWindow()
{
	m_hAccel = ::LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR1));
	pWatch = (CWatch*)this->GetParent();
	GetHeaderCtrl()->EnableWindow(FALSE);
	CListCtrl::PreSubclassWindow();
}

BOOL CMyListCtrl::PreTranslateMessage(MSG* pMsg)
{
	if (::TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
		return TRUE;

	return CListCtrl::PreTranslateMessage(pMsg);
}


void CMyListCtrl::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	ModifyStyle(WS_HSCROLL, 0);
	CListCtrl::OnNcCalcSize(bCalcValidRects, lpncsp);
}


BOOL CMyListCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	HD_NOTIFY *pHDN = (HD_NOTIFY*)lParam;

	if (pHDN->hdr.code == HDN_BEGINTRACKW || pHDN->hdr.code == HDN_BEGINTRACKA) {
		*pResult = TRUE;                // disable tracking
		return TRUE;                    // Processed message
	}

	return CListCtrl::OnNotify(wParam, lParam, pResult);
}


void CopyToClipBoard(CString output) {
	const size_t len = output.GetLength() + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
	memcpy(GlobalLock(hMem), (const WCHAR*)output, len * sizeof(WCHAR));
	GlobalUnlock(hMem);
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hMem);
	CloseClipboard();
}


void CMyListCtrl::SelectAll() {
	for (int i = 0; i < GetItemCount(); i++)
		SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
}


void CMyListCtrl::CopyListSelectedToClipBoard() {
	CString str;
	int nCount = GetSelectedCount();
	if (nCount == 0)
		return;
	POSITION pos = GetFirstSelectedItemPosition();
	for (int i = 0; i < nCount; i++) {
		int Index = GetNextSelectedItem(pos);
		str += GetItemText(Index, INDEX_VALUE);
		if (i < nCount - 1)
			str += L"\r\n";
	}
	CopyToClipBoard(str);
}

void CMyListCtrl::ResetFilter() {
	POSITION pos = ListIndexMap.GetStartPosition();
	while (pos) {
		CString key;
		int value;
		ListIndexMap.GetNextAssoc(pos, key, value);
		if (value == -1)
			ListIndexMap.RemoveKey(key);
	}
}

void CMyListCtrl::Clear()
{
	POSITION pos = ListIndexMap.GetStartPosition();
	while (pos) {
		CString key;
		int value;
		ListIndexMap.GetNextAssoc(pos, key, value);
		if (value != -1)
			ListIndexMap.RemoveKey(key);
	}

	DeleteAllItems();
}

void CMyListCtrl::AddItem(CString szValue) {
	if (m_bSingleMode) {
		if (ListIndexMap[szValue] == -1)
			return;
		if (GetItemCount() == 0)
			InsertItem(0, L"");
		//Add count and set text
		CString szCount = GetItemText(0, INDEX_COUNT);
		UINT uCnt = _wtoi(szCount);
		szCount.Format(L"%d", uCnt + 1);
		SetItemText(0, INDEX_COUNT, szCount);
		SetItemText(0, INDEX_VALUE, szValue);
		return;
	}

	int Index;
	if (ListIndexMap.Lookup(szValue, Index)) {
		if (Index == -1)
			return;
		//Add count
		CString szCount = GetItemText(Index, INDEX_COUNT);
		UINT uCnt = _wtoi(szCount);
		szCount.Format(L"%d", uCnt + 1);
		SetItemText(Index, INDEX_COUNT, szCount);
		return;
	}
	else {
		Index = GetItemCount();
		if (Index < 1000) {
			//Create new item
			int n = InsertItem(Index, L"");
			SetItemText(n, INDEX_COUNT, L"1");
			SetItemText(n, INDEX_VALUE, szValue);
			ListIndexMap[szValue] = Index;
			return;
		}
	}
}

void CMyListCtrl::DeleteSelectedItem() {
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos != NULL) {
		int nItem = GetNextSelectedItem(pos);
		ListIndexMap[GetItemText(nItem, INDEX_VALUE)] = -1;
		DeleteItem(nItem);
		pos = GetFirstSelectedItemPosition();
	}

	for (int i = 0; i < GetItemCount(); i++)
		ListIndexMap[GetItemText(i, INDEX_VALUE)] = i;
}

void __stdcall SetHexViewAddress(LPVOID Address) {
	char szScript[0x100];
	sprintf(szScript,
		"local mv=getMemoryViewForm()\n"
		"mv.HexadecimalView.Address = 0x%I64X\n"
		"mv.show()\n", (DWORD64)Address);

	//This should open a memoryview window at specified address
	lua_State *lua = (lua_State*)CheatEngine.GetLuaState();
	if (!lua)
		return;

	int i = lua_gettop(lua);
	luaL_dostring(lua, szScript);
	lua_settop(lua, i);	//restore
}

void __stdcall SetDisassemblerViewAddress(LPVOID Address) {
	char szScript[0x100];
	sprintf(szScript,
		"local mv=getMemoryViewForm()\n"
		"mv.Disassemblerview.SelectedAddress = 0x%I64X\n"
		"mv.show()\n", (DWORD64)Address);

	//This should open a memoryview window at specified address
	lua_State *lua = (lua_State*)CheatEngine.GetLuaState();
	if (!lua)
		return;

	int i = lua_gettop(lua);
	luaL_dostring(lua, szScript);
	lua_settop(lua, i);	//restore
}

BOOL GetHexViewAddress(CString Input, PUINT64 pAddress) {
	int Index = Input.Find(L"0x");
	if (Index == -1)
		return FALSE;

	CString AddressPart = (const WCHAR*)Input + 2;

	int Len = AddressPart.GetLength();


	if (ParseInfo::bX86 && Len != 8)
		return FALSE;
	if (!ParseInfo::bX86 && Len != 8 && Len != 16)
		return FALSE;

	WCHAR* pEnd;
	*pAddress = _wcstoui64(AddressPart, &pEnd, 16);

	if (errno == 0 && *pEnd == 0)
		return TRUE;

	return FALSE;
}

BOOL GetDisassemblerViewAddress(CString Input, PUINT64 pAddress) {
	int Index = Input.Find(L" - ");
	if (Index == -1)
		return FALSE;

	CString AddressPart = Input.Left(Index);

	WCHAR* pEnd;
	*pAddress = _wcstoui64(AddressPart, &pEnd, 16);

	if (errno == 0 && *pEnd == 0)
		return TRUE;

	return FALSE;
}

void CMyListCtrl::SetViewAddress() {
	int nItem = GetSelectionMark();
	if (nItem == -1)
		return;

	CString szValue = GetItemText(nItem, INDEX_VALUE);

	DWORD64 Address;
	if (GetHexViewAddress(szValue, &Address))
		CheatEngine.MainThreadCall(SetHexViewAddress, (LPVOID)Address);
	if (GetDisassemblerViewAddress(szValue, &Address))
		CheatEngine.MainThreadCall(SetDisassemblerViewAddress, (LPVOID)Address);
}

void CMyListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	SetViewAddress();
	*pResult = 0;
}

void CMyListCtrl::OnAcceleratorCtrlA() {
	SelectAll();
}

void CMyListCtrl::OnAcceleratorCtrlC() {
	CopyListSelectedToClipBoard();
}

void CMyListCtrl::OnAcceleratorCtrlG() {
	SetViewAddress();
}

void CMyListCtrl::OnAcceleratorCtrlR() {
	ResetFilter();
}

void CMyListCtrl::OnAcceleratorCtrlT() {
	ToggleMode();
}

void CMyListCtrl::ToggleMode() {
	Clear();
	m_bSingleMode = !m_bSingleMode;
	pWatch->SetWindowPos(NULL, 0, 0, pWatch->m_DlgRect.Width(), pWatch->m_DlgRect.Height(), SWP_NOMOVE);
}

void CMyListCtrl::HandleRClick() {
	CPoint CurrentPosition;
	GetCursorPos(&CurrentPosition);
	ScreenToClient(&CurrentPosition);
	int nIndex = HitTest(CurrentPosition);

	GetCursorPos(&CurrentPosition);

	CMenu MenuPopup;
	MenuPopup.CreatePopupMenu();

	if (nIndex != -1) {
		DWORD64 Address;
		CString ItemStr = GetItemText(nIndex, INDEX_VALUE);

		MenuPopup.AppendMenuW(MF_STRING, 1, L"Copy to clipboard (Ctrl + C)");

		if (GetHexViewAddress(ItemStr, &Address))
			MenuPopup.AppendMenuW(MF_STRING, 2, L"Go to hex viewer (Ctrl + G)");
		if (GetDisassemblerViewAddress(ItemStr, &Address))
			MenuPopup.AppendMenuW(MF_STRING, 3, L"Go to disassembler (Ctrl + G)");

		MenuPopup.AppendMenuW(MF_STRING, 4, L"Filter this (Delete)");
	}

	MenuPopup.AppendMenuW(MF_STRING, 5, L"Reset filter (Ctrl + R)");
	MenuPopup.AppendMenuW(MF_STRING, 6, L"Clear list (Ctrl + L)");

	if (m_bSingleMode)
		MenuPopup.AppendMenuW(MF_STRING, 7, L"Multi row mode (Ctrl + T)");
	else
		MenuPopup.AppendMenuW(MF_STRING, 7, L"Single row mode (Ctrl + T)");

	int nSelected = MenuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, CurrentPosition.x, CurrentPosition.y, this);

	switch (nSelected) {
	case 1:
		CopyListSelectedToClipBoard();
		break;
	case 2:
	case 3:
		SetViewAddress();
		break;
	case 4:
		DeleteSelectedItem();
		break;
	case 5:
		ResetFilter();
		break;
	case 6:
		Clear();
		break;
	case 7:
		ToggleMode();
		break;
	}
}

void CMyListCtrl::OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	HandleRClick();
	*pResult = 0;
}


void CMyListCtrl::OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	switch (pLVKeyDow->wVKey) {
	case VK_DELETE:
		DeleteSelectedItem();
		break;
	}

	*pResult = 0;
}




