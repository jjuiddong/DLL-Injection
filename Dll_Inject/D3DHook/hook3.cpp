
#include "stdafx.h"
#include "hook3.h"

namespace nhook3
{
	HRESULT __cdecl PresentHook(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect,
		HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

	FARPROC g_original = (FARPROC)NULL;
	LPDIRECT3D9 g_d3d9 = NULL;
	void * pCreateDevice = NULL; //IDirect3D9:: CreateDevice function address pointer
	BYTE CreateDevice_Begin[5]; // CreateDevice IDirect3D9 : ; // save entrance byte
	BYTE Present_Begin[5]; //Present IDirect3DDevice9 : ; // to save the entrance of 5 bytes
	BYTE g_OrigCode[5];

	DWORD PresentFunc, Present_Jump;
	HRESULT(WINAPI* Present_Pointer)(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect,
		HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
	DWORD D3D9VTable(DWORD base);
	void *DetourCreate(BYTE *src, const BYTE *dst, const int len);
	void dump_buffer(IDirect3DDevice9 *Device);
}

using namespace nhook3;


bool hook3()
{
	dbg::RemoveLog();
	dbg::Log("hook3 \n");

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

	PDWORD dwD3DVTable;
	DWORD codeAddr = 0;
	do
	{
		Sleep(100);
		//*(DWORD*)&dwD3DVTable = *(DWORD*)D3D9VTable((DWORD)u);
		codeAddr = D3D9VTable((DWORD)u);
		*(DWORD*)&dwD3DVTable = *(DWORD*)codeAddr;
	} while (!dwD3DVTable);

	dbg::Log("find vtable = %x, addr = %x\n", codeAddr, *(DWORD*)codeAddr);


	PresentFunc = dwD3DVTable[17];
	Present_Jump = (PresentFunc + 0x5);
	*(PDWORD)(&Present_Pointer) = (DWORD)dwD3DVTable[17];

	dbg::Log("find vtable present ptr = %x  \n", dwD3DVTable[17]);

	//Save out the original bytes
	for (int i = 0; i < 5; i++)
		g_OrigCode[i] = *((BYTE*)PresentFunc + i);

	DetourCreate((BYTE*)PresentFunc, (BYTE*)PresentHook, 5);

	return true;
}

DWORD nhook3::D3D9VTable(DWORD base)
{
	DWORD dwObjBase = (DWORD)base;
	//while (dwObjBase++ < dwObjBase + 0x127850)
	while (dwObjBase++ < base + 0x127850)
	{
		// pattern match
		// C7 06 ? ? ? ?		MOV DWORD PTR DS:[ESI],724D869C
		// 89 86 ? ? ? ?		MOV DWORD PTR DS:[ESI+3108],EAX 
		// 89 86 ? ? ? ?		MOV DWORD PTR DS:[ESI+3100],EAX
		if ((*(WORD*)(dwObjBase + 0x00)) == 0x06C7 && 
			(*(WORD*)(dwObjBase + 0x06)) == 0x8689 && 
			(*(WORD*)(dwObjBase + 0x0C)) == 0x8689)
		{
			dwObjBase += 2;
			break;
		}
	}
	return (dwObjBase);
}


void *nhook3::DetourCreate(BYTE *src, const BYTE *dst, const int len)
{
	BYTE *jmp = (BYTE*)malloc(len + 5);
	DWORD dwBack;

	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwBack);
	memcpy(jmp, src, len);
	jmp += len;
	jmp[0] = 0xE9;
	*(DWORD*)(jmp + 1) = (DWORD)(src + len - jmp) - 5;
	src[0] = 0xE9;
	*(DWORD*)(src + 1) = (DWORD)(dst - src) - 5;
	for (int i = 5; i < len; i++)  src[i] = 0x90;
	VirtualProtect(src, len, dwBack, &dwBack);
	return (jmp - len);
}


HRESULT __cdecl nhook3::PresentHook(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect,
	HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
	//dbg::Log("present hook() \n");

	dump_buffer(pDevice);

	__asm
	{
		JMP[Present_Jump]
	}

	return S_OK;
}


void nhook3::dump_buffer(IDirect3DDevice9 *Device)
{
	IDirect3DSurface9* pRenderTarget = NULL;
	IDirect3DSurface9* pDestTarget = NULL;
	const char file[] = "Pickture.bmp";
	// sanity checks.
	if (Device == NULL)
		return;

	// get the render target surface.
	HRESULT hr = Device->GetRenderTarget(0, &pRenderTarget);
	// get the current adapter display mode.
	//hr = pDirect3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT,&d3ddisplaymode);
	D3DSURFACE_DESC desc;
	pRenderTarget->GetDesc(&desc);

	// create a destination surface.
	hr = Device->CreateOffscreenPlainSurface(
		desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pDestTarget, NULL);
	//copy the render target to the destination surface.
	hr = Device->GetRenderTargetData(pRenderTarget, pDestTarget);
	//save its contents to a bitmap file.
	hr = D3DXSaveSurfaceToFileA(file,
		D3DXIFF_BMP,
		pDestTarget,
		NULL,
		NULL);

	// clean up.
	pRenderTarget->Release();
	pDestTarget->Release();
}
