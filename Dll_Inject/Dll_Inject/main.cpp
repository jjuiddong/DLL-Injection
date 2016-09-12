
#include <iostream>
#include <windows.h>
#include <string>
#include <TlHelp32.h>

using namespace std;

wstring str2wstr(const std::string &str)
{
	int len;
	int slength = (int)str.length() + 1;
	len = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	::MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}


DWORD FindProcessID(const wstring &processName)
{
	DWORD dwPID = 0xFFFFFFFF;
	HANDLE hSnapShot = INVALID_HANDLE_VALUE;
	PROCESSENTRY32 pe;

	pe.dwSize = sizeof(pe);
	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);

	Process32First(hSnapShot, &pe);
	do
	{
		if (processName == pe.szExeFile)
		{
			dwPID = pe.th32ProcessID;
			break;
		}
	} while (Process32Next(hSnapShot, &pe));

	CloseHandle(hSnapShot);

	return dwPID;
}


BOOL InjectDll(DWORD dwPID, const wstring &szDllName)
{
	HANDLE hProcess = OpenProcess(
		PROCESS_ALL_ACCESS,
		FALSE, dwPID);
	if (!hProcess)
		return FALSE;

// 	HANDLE hToken;
// 	TOKEN_PRIVILEGES tp;
// 	tp.PrivilegeCount = 1;
// 	LookupPrivilegeValue(NULL, L"SeDebugPrivilege", &tp.Privileges[0].Luid);
// 	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
// 	OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken);
// 	AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, NULL, NULL);
// 	CloseHandle(hToken);

	LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL,
		szDllName.length() * 2 + 2,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE);
	if (!pRemoteBuf)
		return FALSE;

	if (!WriteProcessMemory(hProcess,
		pRemoteBuf,
		szDllName.c_str(),
		szDllName.length() * 2 + 2, NULL))
		return FALSE;

	HMODULE hMod = GetModuleHandle(L"kernel32.dll");
	LPTHREAD_START_ROUTINE pThreadProc =
		(LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "LoadLibraryW");

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
		pThreadProc, pRemoteBuf, 0, NULL);
	DWORD err = GetLastError();
	cout << err << endl;
	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hProcess);
	return TRUE;
}


void main(int argc, char *argv[])
{
	cout << "commandline <exe file name> <dll file name>" << endl;
	if (argc < 3)
		return;

	wstring exeName = str2wstr(argv[1]);
	string tmpDll = argv[2];
	char dllFullPath[MAX_PATH];
	GetFullPathNameA(tmpDll.c_str(), MAX_PATH, dllFullPath, NULL);
	wstring dllName = str2wstr(dllFullPath);

	cout << "exe = ";
	wcout << exeName << endl;
	cout << "dll = ";
	wcout << dllName << endl;

	DWORD dwPID = FindProcessID(exeName);
	if (dwPID == 0xFFFFFFFF)
	{
		cout << "not found exe" << endl;
		return;
	}

	InjectDll(dwPID, dllName);

	getchar();
}
