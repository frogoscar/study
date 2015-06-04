
// ThreadPoolDemoDlg.h : header file
//
#pragma once

#include "OutputLogger.h"
#include "ThreadPool.h"
#include <vector>

using namespace TP;

typedef std::vector<AbstractRequest*> REQUEST_VECTOR;

// CThreadPoolDemoDlg dialog
class CThreadPoolDemoDlg : public CDialog
{
// Construction
public:
	CThreadPoolDemoDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_THREADPOOLDEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
    void ClearRequests();
    afx_msg void OnBnClickedCreate();
    afx_msg void OnBnClickedDestroy();
    afx_msg void OnBnClickedPushRequest();
    afx_msg void OnBnClickedCancel();

private:
    short m_sThreadCount;
    OutputLogger m_Logger;
    ThreadPool m_ThreadPool;
    REQUEST_VECTOR m_RequestList;
public:
    short m_sReqCount;
};
