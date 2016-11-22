#pragma once
#include "afxwin.h"
#include "afxeditbrowsectrl.h"

class CInjectorDlg : public CDialogEx
{
public:
	CInjectorDlg(CWnd* pParent = NULL);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_INJECTOR_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	void UpdateProcessList();
	BOOL InjectDll(DWORD dwPID, const CString &szDllName);


protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CListBox m_ProcessList;
	afx_msg void OnBnClickedButtonRefresh();
	afx_msg void OnBnClickedButtonInject();
	CMFCEditBrowseCtrl m_browserDLL;
};
