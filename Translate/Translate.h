// Translate.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CTranslateApp:
// See Translate.cpp for the implementation of this class
//

class CTranslateApp : public CWinApp
{
public:
	CTranslateApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CTranslateApp theApp;