// approval_list.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Capproval_listApp:
// �йش����ʵ�֣������ approval_list.cpp
//

class Capproval_listApp : public CWinApp
{
public:
	Capproval_listApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Capproval_listApp theApp;