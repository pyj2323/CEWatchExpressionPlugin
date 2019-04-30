#pragma once
#include "afxcmn.h"
#include "Watch.h"

#define INDEX_COUNT 0
#define INDEX_VALUE 1

class CMyListCtrl : public CListCtrl
{
public:
	class CWatch* pWatch;

	HACCEL m_hAccel;

	BOOL m_bSingleMode;
	CMap<CString, LPCTSTR, int, int> ListIndexMap;

	CMyListCtrl();
	~CMyListCtrl();

	void SetViewAddress();
	void SelectAll();
	void CopyListSelectedToClipBoard();
	void Clear();
	void AddItem(CString szValue);
	void DeleteSelectedItem();
	void ResetFilter();
	void ToggleMode();
	void HandleRClick();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnAcceleratorCtrlA();
	afx_msg void OnAcceleratorCtrlC();
	afx_msg void OnAcceleratorCtrlG();
	afx_msg void OnAcceleratorCtrlR();
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnAcceleratorCtrlT();
	afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult);
	//	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PreSubclassWindow();
	afx_msg void OnAcceleratorCtrlL();
};