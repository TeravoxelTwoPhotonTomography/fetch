// TranslateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Translate.h"
#include "TranslateDlg.h"

#include "devices/DiskStream.h"
#include "frame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTranslateDlg dialog




CTranslateDlg::CTranslateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTranslateDlg::IDD, pParent)
  , format(_T("raw"))
  {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTranslateDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_CBString(pDX, IDC_COMBO_OUT_FORMATS, format);
DDX_Control(pDX, IDC_LIST_IN_FILE, ctl_in_list);
  }

BEGIN_MESSAGE_MAP(CTranslateDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
  ON_BN_CLICKED(IDC_COMMAND_TRANSLATE, &CTranslateDlg::OnBnClickedCommandTranslate)
  ON_WM_DROPFILES()
ON_WM_CREATE()
END_MESSAGE_MAP()


// CTranslateDlg message handlers

BOOL CTranslateDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTranslateDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTranslateDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void map_file_name(asynq *sourceq, char *outname, char *inname)
  // * agent that fill sourceq must already be running
{ //create terminal part of modified filename and extension.
  char endpart[MAX_PATH];
  { size_t n[3];
    Frame *buf = (Frame*)Asynq_Token_Buffer_Alloc(sourceq);
    Asynq_Peek(sourceq,(void**)&buf,sourceq->q->buffer_size_bytes); // a bit inefficient.  Copies the whole frame just to get the header.    
    buf->get_shape(n);
    sprintf(endpart,"_%dx%dx%d.%s",n[2],n[1],n[0],TypeStrFromID(buf->rtti));
    Asynq_Token_Buffer_Free(buf);
  }

  // assemble output filename
  char *cur,*end = inname + strlen(inname);
  cur = end;
  while(cur-->inname && *cur!='.');      // rindex - looking for first extension 
  if(cur!=inname) *cur = '\0';
  strcpy(outname,inname);
  strcat(outname,endpart);
}

DWORD WINAPI readproc(LPVOID lparam)
{ USES_CONVERSION;
  struct T {TCHAR name[MAX_PATH];};
  asynq *q = (asynq*)(lparam);
  T args = {0};
  if(!Asynq_Pop_Copy_Try(q, &args, sizeof(T)))
  { warning("%s(%d): tCould not pop arguments from queue.\r\n",__FILE__,__LINE__);
    return 1;
  }
  
  { // work  
    char* inname = T2A(args.name);
    char outname[MAX_PATH] = {'\0'};
    fetch::device::DiskStreamMessage      in;
    fetch::device::DiskStreamMessageAsRaw out;
    in.open(T2A(args.name),"r");
    map_file_name(in.out->contents[0],outname,inname);
    fetch::Agent::connect(&out,0,&in,0);
    Guarded_Assert(out.open(outname,"w"));
    if( !in.wait_till_stopped(INFINITE) )
      warning("%s(%d): Wait for read - timed out\r\n",__FILE__,__LINE__);
    out.close();
    in.close();
  }
  return 0;
}

void CTranslateDlg::OnBnClickedCommandTranslate()
  { 
    // TODO: Add your control notification handler code here
    struct T {TCHAR name[MAX_PATH];};
    static asynq *q = NULL;    
    int nFiles = ctl_in_list.GetCount();
    CString name;
    
    if(!q)
      q = Asynq_Alloc(nFiles,sizeof(T));
    
    for(int i=0; i<nFiles; ++i)
    { T args;
      ctl_in_list.GetText(i,args.name);
      if(!Asynq_Push_Copy(q, &args, sizeof(T), TRUE))
      { warning("%s(%d): Could not push request arguments to queue.",__FILE__,__LINE__);
        break;
      }
      Guarded_Assert_WinErr( QueueUserWorkItem(&readproc, (void*)q, NULL /*default flags*/));      
    }
  }

void CTranslateDlg::OnDropFiles(HDROP hDropInfo)
  {
  // TODO: Add your message handler code here and/or call default
  int nFiles = ::DragQueryFile(hDropInfo,(UINT)-1,NULL,0);
  TCHAR name[MAX_PATH] = {'\0'};

  ctl_in_list.InitStorage(nFiles,MAX_PATH);
  for(int i=0; i<nFiles; ++i)
  { ::DragQueryFile(hDropInfo,i,name,MAX_PATH);
    ctl_in_list.AddString(name);
  }
  ::DragFinish(hDropInfo);
  CDialog::OnDropFiles(hDropInfo);
  
  //
  // Set HorizontalExtent to fit longest string
  //
  { // Find the longest string in the list box.
    CString str;
    CSize   sz;
    int     dx=0;
    CDC*    pDC = ctl_in_list.GetDC();
    for (int i=0;i < ctl_in_list.GetCount();i++)
    {
       ctl_in_list.GetText( i, str );
       sz = pDC->GetTextExtent(str);

       if (sz.cx > dx)
          dx = sz.cx;
    }
    ctl_in_list.ReleaseDC(pDC);

    // Set the horizontal extent only if the current extent is not large enough.
    if (ctl_in_list.GetHorizontalExtent() < dx)
    {
       ctl_in_list.SetHorizontalExtent(dx);
       ASSERT(ctl_in_list.GetHorizontalExtent() == dx);
    }
  }
  
  }
int CTranslateDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
  {
  if (CDialog::OnCreate(lpCreateStruct) == -1)
    return -1;

  // TODO:  Add your specialized creation code here

  return 0;
  }
