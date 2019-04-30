// Honoka.h : Honoka DLL의 기본 헤더 파일입니다.
//

#pragma once

#ifndef __AFXWIN_H__
#error "PCH에 대해 이 파일을 포함하기 전에 'stdafx.h'를 포함합니다."
#endif

#include "resource.h"		// 주 기호입니다.


// CHonokaApp
// 이 클래스의 구현을 보려면 Honoka.cpp를 참조하십시오.
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
