// Honoka.h : main header file for the HONOKA DLL
//

#if !defined(AFX_HONOKA_H__57A3B721_45E5_4D64_B760_54A4F635D4EF__INCLUDED_)
#define AFX_HONOKA_H__57A3B721_45E5_4D64_B760_54A4F635D4EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CHonokaApp
// See Honoka.cpp for the implementation of this class
//

class CHonokaApp : public CWinApp
{
public:
	CHonokaApp();

// 재정의입니다.
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HONOKA_H__57A3B721_45E5_4D64_B760_54A4F635D4EF__INCLUDED_)
