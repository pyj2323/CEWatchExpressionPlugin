#pragma once
#include "afxwin.h"


// CWatch 대화 상자입니다.

enum OutputType {
	OUTPUT_INTEGER = 0,
	OUTPUT_OPCODE,
	OUTPUT_FLOAT,
	OUTPUT_DOUBLE,
	OUTPUT_STRING,
	OUTPUT_AOB
};

enum IntShowType {
	OUTPUT_DEC_SIGNED = 0,
	OUTPUT_DEC_UNSIGNED,
	OUTPUT_HEX
};

enum StrShowType {
	OUTPUT_ANSI = 0,
	OUTPUT_UTF8,
	OUTPUT_UTF16
};

class CWatch : public CDialog
{
	DECLARE_DYNAMIC(CWatch)

public:
	CWatch(PVOID pAddress, CWnd* pParent = NULL);
	virtual ~CWatch();

	// 대화 상자 데이터입니다.
	enum { IDD = IDD_WATCH };

	PVOID m_pAddress;
	BOOL m_bCapture;
	OutputType m_nType;
	IntShowType m_nIntShowType;
	StrShowType m_nStrShowType;

	BOOL m_bRect;
	CRect m_DlgRect;
	CRect m_ListRect;

	CString m_szExp;
	CString m_szLen;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonState();
	virtual BOOL OnInitDialog();
	CButton m_btnState;
protected:
	afx_msg LRESULT OnGetcontext(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnBnClickedButtonClear();
	CListCtrl m_listLog;
	CEdit m_editExp;
	CComboBox m_comboType;
	CEdit m_editLen;
	CButton m_radioOpt1;
	CButton m_radioOpt2;
	CButton m_radioOpt3;
	afx_msg void OnBnClickedRadioOpt1();
	CStatic m_staticLen;
	afx_msg void OnCbnSelchangeComboType();
	afx_msg void OnBnClickedRadioOpt2();
	afx_msg void OnBnClickedRadioOpt3();
	afx_msg void OnDblclkListLog(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
};
