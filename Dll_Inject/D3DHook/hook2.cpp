
#include "stdafx.h"
#include "hook2.h"


namespace nhook2
{
	//Direct3D hooking functions and data
	unsigned char g_OrigCode[5];          //Original code at start of D3D's Present function 
	unsigned char g_PatchCode[5];         //The patched code we overlay 
	unsigned char* g_pPatchAddr = NULL;   //Address of the patch (start of the real Present function)
	void HookIntoD3D();
	void ApplyPatch();
	void RemovePatch();

	typedef HRESULT(_stdcall *RealPresentFuncType)(void*, const RECT*, const RECT*, HWND, const RGNDATA*);
	RealPresentFuncType RealPresent;
	HRESULT _stdcall Present(void *pThis, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
}

using namespace nhook2;


//This function finds the address to the d3d9.dll Present function, which we know FSX has already 
//loaded into this process, and patches the code with a JMP to our Present function. In our Present 
//function we first call g_GUI.OnFSXPresent(), then we undo the patch with the original code, call 
//the real d3d Present function, and reinstate the patch. Note this needs to be in the same process
//as FSX (to get the same d3d9.dll instance FSX is using), which is why this is a DLL addon and not
//an EXE. 
void nhook2::HookIntoD3D()
{
	//Create a temporary direct 3D object and get its IDirect3DDevice9 interface. This is a different 
	//"instance," but the virtual table will point to the function within d3d9.dll also used by FSX's 
	//instance.
	LPDIRECT3D9 p = Direct3DCreate9(D3D_SDK_VERSION);
	if (!p)
		return;

	HWND g_hWnd = FindWindowA(NULL, "Sample");
	dbg::Log("hwnd = %x\n", g_hWnd);

	D3DPRESENT_PARAMETERS presParams;
	ZeroMemory(&presParams, sizeof(presParams));
	presParams.Windowed = TRUE;
	presParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	IDirect3DDevice9 *pI = NULL;
	HRESULT hr;
	hr = p->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, g_hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &presParams, &pI);
	if (!pI)
	{
		p->Release();
		dbg::Log("fail initialize finish \n");
		return;
	}

	//Get the pointer to the virtual table for IDirect3DDevice9 (pI is a pointer to a pointer to the Vtable) 
	PVOID* pVTable = (PVOID*)*((DWORD*)pI);

	//Set permissions so we can read it (117 = size of whole table)
	DWORD dwOldProtect;
	VirtualProtect(pVTable, sizeof(void *) * 117, PAGE_READWRITE, &dwOldProtect);

	//Get the address to the (real) Present function (17th function listed, starting at 0 -- see definition 
	//of IDirect3DDevice9). 
	RealPresent = (RealPresentFuncType)pVTable[17];

	//Calculate the offset from the real function to our hooked function. Constant 5 is because JMP
	//offset is from start of next instruction (5 bytes later)
	DWORD offset = (DWORD)Present - (DWORD)RealPresent - 5;

	//Create the patch code: JMP assembly instruction (0xE9) followed by relative offset 
	g_PatchCode[0] = 0xE9;
	*((DWORD*)&g_PatchCode[1]) = offset;
	g_pPatchAddr = (unsigned char*)RealPresent;

	//Set permission to allow reading/write/execute
	VirtualProtect(g_pPatchAddr, 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//Save out the original bytes
	for (int i = 0; i < 5; i++)
		g_OrigCode[i] = *(g_pPatchAddr + i);

	//Copy in our patch code
	ApplyPatch();

	//Delete dummy D3D objects
	pI->Release();
	p->Release();

	dbg::Log("hook initialize finish \n");

	return;
}

//Our hooked function. FSX calls this thinking it's calling IDirect3DDevice9::Present
HRESULT _stdcall nhook2::Present(void *pThis, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
	IDirect3DDevice9 *pI = (IDirect3DDevice9 *)pThis;

	//Call our main class
	//g_GUI.OnFSXPresent(pI);
	dbg::Log("present \n");

	//Call the real "Present"
	RemovePatch();
	HRESULT hr = RealPresent(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	ApplyPatch();

	return hr;
}

//Modify d3d9.dll Present function to call ours instead
void nhook2::ApplyPatch()
{
	for (int i = 0; i < 5; i++)
		*(g_pPatchAddr + i) = g_PatchCode[i];

	//FlushInstructionCache(GetCurrentProcess(), g_pPatchAddr, 5); //apparently not necessary on x86 and x64 CPU's?
	return;
}

//Restore d3d9.dll's Present function to its original code
void nhook2::RemovePatch()
{
	for (int i = 0; i < 5; i++)
		*(g_pPatchAddr + i) = g_OrigCode[i];

	//FlushInstructionCache(GetCurrentProcess(), g_pPatchAddr, 5);
	return;
}

bool hook2()
{
	dbg::RemoveLog();
	dbg::Log("hook2 \n");

	HookIntoD3D();

	return true;
}

