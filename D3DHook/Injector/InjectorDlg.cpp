
// InjectorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Injector.h"
#include "InjectorDlg.h"
#include "afxdialogex.h"
#include <TlHelp32.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CInjectorDlg::CInjectorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_INJECTOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CInjectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PROCESS, m_ProcessList);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_DLL, m_browserDLL);
}

BEGIN_MESSAGE_MAP(CInjectorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CInjectorDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CInjectorDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH, &CInjectorDlg::OnBnClickedButtonRefresh)
	ON_BN_CLICKED(IDC_BUTTON_INJECT, &CInjectorDlg::OnBnClickedButtonInject)
END_MESSAGE_MAP()


// CInjectorDlg message handlers

BOOL CInjectorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
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
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	UpdateProcessList();

	return TRUE;
}

void CInjectorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}


void CInjectorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CInjectorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CInjectorDlg::OnBnClickedOk()
{
//	CDialogEx::OnOK();
}


void CInjectorDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}


void CInjectorDlg::UpdateProcessList()
{
	DWORD dwPID = 0xFFFFFFFF;
	HANDLE hSnapShot = INVALID_HANDLE_VALUE;
	PROCESSENTRY32 pe;

	// clear listbox
	const int cnt = m_ProcessList.GetCount();
	for (int i = 0; i < cnt; ++i)
		m_ProcessList.DeleteString(0);

	pe.dwSize = sizeof(pe);
	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);

	Process32First(hSnapShot, &pe);
	do
	{
		const int idx = m_ProcessList.AddString(pe.szExeFile);
		m_ProcessList.SetItemData(idx, pe.th32ProcessID);

	} while (Process32Next(hSnapShot, &pe));

	CloseHandle(hSnapShot);
}


void CInjectorDlg::OnBnClickedButtonRefresh()
{
	UpdateProcessList();
	m_ProcessList.SetFocus();
}


void CInjectorDlg::OnBnClickedButtonInject()
{
	CString dllPath;
	m_browserDLL.GetWindowTextW(dllPath);
	if (dllPath.IsEmpty())
	{
		AfxMessageBox(L"Please Select DLL File");
		return;
	}

	const int selIndex = m_ProcessList.GetCurSel();
	if (selIndex < 0)
	{
		AfxMessageBox(L"Please Select Process");
		return;
	}

	CString exeName;
	m_ProcessList.GetText(selIndex, exeName);
	if (InjectDll((DWORD)m_ProcessList.GetItemData(selIndex), dllPath))
	{
		AfxMessageBox(CString("Success Inject DLL ") + dllPath + L" " + exeName);
	}
	else
	{
		AfxMessageBox(CString(L"Fail Inject DLL ") + dllPath + L" " + exeName);
	}
}


BOOL CInjectorDlg::InjectDll(DWORD dwPID, const CString &szDllName)
{
	HANDLE hProcess = OpenProcess(
		PROCESS_ALL_ACCESS,
		FALSE, dwPID);
	if (!hProcess)
	{
		return FALSE;
	}

	LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL,
		szDllName.GetLength() * 2 + 2,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE);
	if (!pRemoteBuf)
		return FALSE;

	if (!WriteProcessMemory(hProcess,
		pRemoteBuf,
		szDllName,
		szDllName.GetLength() * 2 + 2, NULL))
		return FALSE;

	HMODULE hMod = GetModuleHandle(L"kernel32.dll");
	LPTHREAD_START_ROUTINE pThreadProc =
		(LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "LoadLibraryW");

	if (!pThreadProc)
		return FALSE;

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
		pThreadProc, pRemoteBuf, 0, NULL);
	DWORD err = GetLastError();

	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hProcess);
	return TRUE;
}
