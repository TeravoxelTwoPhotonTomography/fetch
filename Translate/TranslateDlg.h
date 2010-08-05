// TranslateDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CTranslateDlg dialog
class CTranslateDlg : public CDialog
{
// Construction
public:
	CTranslateDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TRANSLATE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:  
  afx_msg void OnBnClickedCommandTranslate();
  afx_msg void OnDropFiles(HDROP hDropInfo);
  CString format;
  CListBox ctl_in_list;
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  };
