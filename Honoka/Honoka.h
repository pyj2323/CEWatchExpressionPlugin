// Honoka.h : Honoka DLL�� �⺻ ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CHonokaApp
// �� Ŭ������ ������ ������ Honoka.cpp�� �����Ͻʽÿ�.
//

class CHonokaApp : public CWinApp
{
public:
	CHonokaApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
