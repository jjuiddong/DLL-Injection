
#include "stdafx.h"
#include "hook1.h"


namespace nhook1
{
// HRESULT __cdecl PresentHook(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect,
// 	HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
	IDirect3D9 * WINAPI Direct3DCreate9Hook(UINT SDKVersion);
	HRESULT CreateDeviceHook(LPDIRECT3D9 d3d9, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
		D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);

	typedef HRESULT(_stdcall *RealCreateDeviceFuncType)(LPDIRECT3D9 d3d9, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, 
		DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);
	RealCreateDeviceFuncType RealCreateDevice;	

	FARPROC g_original = (FARPROC)NULL;
	FARPROC g_hooked = (FARPROC)Direct3DCreate9Hook;
	LPDIRECT3D9 g_d3d9 = NULL;
	void * pCreateDevice = NULL; //IDirect3D9:: CreateDevice function address pointer
	BYTE CreateDevice_Begin[5]; // CreateDevice IDirect3D9 : ; // save entrance byte
	BYTE Present_Begin[5]; //Present IDirect3DDevice9 : ; // to save the entrance of 5 bytes

	unsigned char g_OrigCode[5];          //Original code at start of D3D's Present function 
	unsigned char g_PatchCode[5];         //The patched code we overlay 
	unsigned char* g_pPatchAddr = NULL;   //Address of the patch (start of the real Present function)
}

using namespace nhook1;


bool hook1()
{
	dbg::RemoveLog();

	// get original messageboxa function address
	//
	HMODULE u = GetModuleHandleA("d3d9.dll");
	if (NULL == u)
	{
		u = LoadLibraryA("d3d9.dll");
		if (NULL == u)
		{
			dbg::Log("can not load d3d9.dll gle=0x%08x \n", GetLastError());
			return false;
		}
	}

	g_original = (FARPROC)GetProcAddress(u, "Direct3DCreate9");
	dbg::Log("d3d9.dll module=0x%08x \n", u);
	dbg::Log("Direct3DCreate9 original=0x%x \n", (int)g_original);

	// find IAT and replace it
	// 
	char* base = (char*)GetModuleHandle(NULL);
	if (NULL == base) return FALSE;

	PIMAGE_DOS_HEADER idh = (PIMAGE_DOS_HEADER)base;
	if (IMAGE_DOS_SIGNATURE != idh->e_magic)
	{
		dbg::Log("invalid idh->e_magic \n");
		return false;
	}

	PIMAGE_NT_HEADERS inh = (PIMAGE_NT_HEADERS)(base + idh->e_lfanew);
	if (IMAGE_NT_SIGNATURE != inh->Signature)
	{
		dbg::Log("invalid inh->Signature \n");
		return false;
	}

	PIMAGE_DATA_DIRECTORY idd = &inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	PIMAGE_IMPORT_DESCRIPTOR iid = (PIMAGE_IMPORT_DESCRIPTOR)(base + idd->VirtualAddress);
	for (; iid->OriginalFirstThunk; ++iid)
	{
		char* module_name = (LPSTR)(base + iid->Name);
		if (0 == _strcmpi(module_name, "d3d9.dll")) break;
	}

	if (NULL == iid->Name)
	{
		dbg::Log("no d3d9.dll from IAT -_- \n");
		return false;
	}

	PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)(base + iid->FirstThunk);
	for (; thunk->u1.Function; thunk++)
	{
		FARPROC* ppfn = (FARPROC*)&thunk->u1.Function;

		if (*ppfn == g_original)
		{
			g_original = **ppfn;

			if (!WriteProcessMemory(GetCurrentProcess(), ppfn,
				&g_hooked, sizeof(g_hooked), NULL) && (ERROR_NOACCESS == GetLastError()))
			{
				DWORD dwOldProtect;
				if (VirtualProtect(ppfn, sizeof(g_hooked), PAGE_WRITECOPY, &dwOldProtect))
				{
					//WriteProcessMemory(GetCurrentProcess(), ppfn, &g_hooked, sizeof(g_hooked), NULL);
					*ppfn = g_hooked;
					VirtualProtect(ppfn, sizeof(g_hooked), dwOldProtect, &dwOldProtect);
				}
			}

			dbg::Log("iat hooked. from 0x%08x --> to 0x%08x \n", g_original, g_hooked);
			return true;
		}
	}

	dbg::Log("not found function \n");
	return true;
}


IDirect3D9 * WINAPI nhook1::Direct3DCreate9Hook(UINT SDKVersion)
{
	dbg::Log("Direct3DCreate9Hook() \n");

	g_d3d9 = Direct3DCreate9(SDKVersion);

	PVOID* pVTable = (PVOID*)*((DWORD*)g_d3d9);
	DWORD dwOldProtect;
	VirtualProtect(pVTable, sizeof(void *) * 17, PAGE_READWRITE, &dwOldProtect);
	RealCreateDevice = (RealCreateDeviceFuncType)pVTable[17];
// 
// 	DWORD offset = (DWORD)CreateDeviceHook - (DWORD)RealCreateDevice - 5;
// 
// 	g_PatchCode[0] = 0xE9;
// 	*((DWORD*)&g_PatchCode[1]) = offset;
// 	g_pPatchAddr = (unsigned char*)RealCreateDevice;
 
// 	VirtualProtect(g_pPatchAddr, 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);
// 
// 	//Save out the original bytes
// 	for (int i = 0; i < 5; i++)
// 		g_OrigCode[i] = *(g_pPatchAddr + i);


// 	pCreateDevice = (void*)(DWORD*)((DWORD*)g_d3d9 + 0x40); // IDirect3D9::CreateDevice address pointer
// 															//int* cVtablePtr = (int*)((int*)g_d3d9)[0];
// 															//pCreateDevice = (void*)cVtablePtr[9];
// 	dbg::Log("CreateDevice = 0x%x \n", pCreateDevice);
// 
// 	DWORD oldpro = 0;
// 	memcpy(CreateDevice_Begin, pCreateDevice, 5); // IDirect3D9::save 5 bytes CreateDevice entrance
// 	VirtualProtect(pCreateDevice, 5, PAGE_EXECUTE_READWRITE, &oldpro);
// 	*(BYTE*)pCreateDevice = 0xe9;
// 	*(DWORD*)((BYTE*)pCreateDevice + 1) = (DWORD)CreateDeviceHook - (DWORD)pCreateDevice - 5;
// 	VirtualProtect(pCreateDevice, 5, oldpro, &oldpro);

	return g_d3d9;
}


HRESULT CreateDeviceHook(
	LPDIRECT3D9 d3d9,
	UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
	DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
	IDirect3DDevice9** ppReturnedDeviceInterface)
{
	dbg::Log("CreateDeviceHook() \n");

// 	HRESULT ret = d3d9->CreateDevice( // create equipment
// 		Adapter,
// 		DeviceType,
// 		hFocusWindow,
// 		BehaviorFlags,
// 		pPresentationParameters,
// 		ppReturnedDeviceInterface);

//	return ret;
	return S_FALSE;
}
