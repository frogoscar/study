
// ThreadPoolDemoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ThreadPoolDemo.h"
#include "ThreadPoolDemoDlg.h"
#include "Request.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CThreadPoolDemoDlg dialog




CThreadPoolDemoDlg::CThreadPoolDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CThreadPoolDemoDlg::IDD, pParent)
    , m_sThreadCount(1)
    , m_sReqCount(1)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CThreadPoolDemoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_COUNT, m_sThreadCount);
    DDX_Text(pDX, IDC_EDIT_REG_COUNT, m_sReqCount);
}

BEGIN_MESSAGE_MAP(CThreadPoolDemoDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BTN_CREATE, &CThreadPoolDemoDlg::OnBnClickedCreate)
    ON_BN_CLICKED(IDC_BTN_DESTROY, &CThreadPoolDemoDlg::OnBnClickedDestroy)
    ON_BN_CLICKED(IDC_BTN_POST, &CThreadPoolDemoDlg::OnBnClickedPushRequest)
    ON_BN_CLICKED(IDC_BTN_CANCEL, &CThreadPoolDemoDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CThreadPoolDemoDlg message handlers

BOOL CThreadPoolDemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CThreadPoolDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CThreadPoolDemoDlg::OnPaint()
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
HCURSOR CThreadPoolDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


/** 
 * Create thread pool
 */
void CThreadPoolDemoDlg::OnBnClickedCreate()
{
    UpdateData();
    if( !m_ThreadPool.Create( m_sThreadCount, &m_Logger ))
    {
        AfxMessageBox( _T( "Thread pool creation failed." ));
        return;
    }
}


/** 
 * Destroy thread pool
 */
void CThreadPoolDemoDlg::OnBnClickedDestroy()
{
    if( !m_ThreadPool.Destroy())
    {
        AfxMessageBox( _T( "Thread pool destruction failed." ));
        return;
    }
    ClearRequests();
}


/** 
 * Post request to thread pool
 */
void CThreadPoolDemoDlg::OnBnClickedPushRequest()
{
    UpdateData();
    for( int nIndex = 0; nIndex < m_sReqCount; nIndex ++ )
    {
        Request* pReq = new Request();
        if( NULL == pReq )
        {
            AfxMessageBox( _T( "Failed to create request." ));
            return;
        }
        pReq->SetRequestID( nIndex );
        if( !m_ThreadPool.PostRequest( pReq ))
        {
            AfxMessageBox( _T( "PostRequest failed." ));
            delete pReq;
            return;
        }
        m_RequestList.push_back( pReq );
    }
}


/** 
 * Cancel all processing requests.
 */
void CThreadPoolDemoDlg::OnBnClickedCancel()
{
    for( REQUEST_VECTOR::iterator itrPos = m_RequestList.begin() ; itrPos != m_RequestList.end(); ++itrPos )
    {
        AbstractRequest* pReq = *itrPos;
        if( NULL != pReq )
        {
            pReq->Abort();
        }
    }
}


/** 
 * Delete all requests
 */
void CThreadPoolDemoDlg::ClearRequests()
{
    REQUEST_VECTOR::iterator itrPos = m_RequestList.begin();
    for( ; itrPos != m_RequestList.end(); ++itrPos )
    {
        AbstractRequest* pReq = *itrPos;
        delete pReq;
    }
    m_RequestList.clear();
}