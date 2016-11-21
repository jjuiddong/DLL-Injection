
#include "stdafx.h"
#include "hook3_64.h"

namespace nhook3
{
	void PresentHook(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect,
		HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

	FARPROC g_original = (FARPROC)NULL;
	LPDIRECT3D9 g_d3d9 = NULL;
	void * pCreateDevice = NULL; //IDirect3D9:: CreateDevice function address pointer
//	BYTE CreateDevice_Begin[5]; // CreateDevice IDirect3D9 : ; // save entrance byte
//	BYTE Present_Begin[5]; //Present IDirect3DDevice9 : ; // to save the entrance of 5 bytes
//	BYTE g_OrigCode[5];

	__int64 PresentFunc, Present_Jump;
	HRESULT(WINAPI* Present_Pointer)(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect,
		HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
	__int64 D3D9VTable(__int64 base);
	void *DetourCreate(BYTE *src, const BYTE *dst, const int len);
	void dump_buffer(IDirect3DDevice9 *Device);
}

using namespace nhook3;


__int64 nhook3::D3D9VTable(__int64 base)
{
	__int64 dwObjBase = (__int64)base;
	//while (dwObjBase++ < dwObjBase + 0x127850)
	while (dwObjBase++ < base + 0x127850)
	{
		// pattern match
		//	48 8D 05 1F DC FE FF lea         rax, [7FFC351A8178h]
		// 48 89 03             mov         qword ptr[rbx], rax
		//	33 C0                xor         eax, eax
		if ((*(WORD*)(dwObjBase + 0x00)) == 0x8D48 &&
			(*(WORD*)(dwObjBase + 0x07)) == 0x8948 &&
			(*(WORD*)(dwObjBase + 0x0A)) == 0xC033)
		{
			//dwObjBase += 3;
			return dwObjBase;
			//break;
		}
	}
	//return (dwObjBase);
	return 0;
}


bool hook3()
{
	dbg::RemoveLog();
	dbg::Log("hook3 \n");

	MessageBoxA(NULL, "InjectDll Success", "Msg", MB_OK);

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

	__int64 *dwD3DVTable;
	__int64 codeAddr = 0;
	__int64 finalAddr = 0;
	do
	{
		Sleep(100);
		//*(DWORD*)&dwD3DVTable = *(DWORD*)D3D9VTable((DWORD)u);
		codeAddr = D3D9VTable((__int64)u);

		if (!codeAddr)
		{
			dbg::Log("not found d3d9 vtable\n");
			return false;
		}

		const int operand = *(int*)(codeAddr + 3) + 7;
		finalAddr = codeAddr + operand;

		//*(__int64*)&dwD3DVTable = *(__int64*)finalAddr;
		dwD3DVTable = (__int64*)finalAddr;
	} while (!dwD3DVTable);

	dbg::Log("dump\n");
	BYTE *dumpPtr = (BYTE*)codeAddr;
	for (int i=0; i < 7; ++i)
		dbg::Log("%2x ", dumpPtr[i]);
	dbg::Log("\n");
	for (int i = 0; i < 3; ++i)
		dbg::Log("%2x ", dumpPtr[i+7]);
	dbg::Log("\n");
	for (int i = 0; i < 2; ++i)
		dbg::Log("%2x ", dumpPtr[i+7+3]);
	dbg::Log("\n");

	dbg::Log("code address = %I64x\n", codeAddr);
	dbg::Log("find vtable = %I64x, pointer = %I64x \n", dwD3DVTable, finalAddr);

	PresentFunc = dwD3DVTable[17];
	Present_Jump = (PresentFunc + 15);
	*(__int64*)(&Present_Pointer) = (__int64)dwD3DVTable[17];
	
	dbg::Log("find vtable present ptr = %I64x  \n", dwD3DVTable[17]);

	//Save out the original bytes
//	for (int i = 0; i < 5; i++)
//		g_OrigCode[i] = *((BYTE*)PresentFunc + i);

	DetourCreate((BYTE*)PresentFunc, (BYTE*)PresentHook, 15);

	return true;
}


// x64 absolute address jump
void *nhook3::DetourCreate(BYTE *src, const BYTE *dst, const int len)
{
	DWORD dwBack;

	const int result = VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &dwBack);

	// mov rax, 64bit address (10 bytes)
	src[0] = 0x48;
	src[1] = 0xb8;
	*(__int64*)(src + 2) = (__int64)dst;

	// jmp rax (2 bytes)
	src[10] = 0xff;
	src[11] = 0xe0;

	for (int i = 12; i < len; i++)
		src[i] = 0x90;

	VirtualProtect(src, len, dwBack, &dwBack);
	return NULL;
}

#ifdef _X64
	extern "C" int Jump64(__int64 arg1, __int64 arg2, __int64 arg3, __int64 arg4, __int64 arg5, __int64 address);
#endif

	
void nhook3::PresentHook(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect,
	HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
	//dbg::Log("present hook() \n");

	dump_buffer(pDevice);

#ifdef _X64
	Jump64((__int64)pDevice, (__int64)pSourceRect, (__int64)pDestRect, 
		(__int64)hDestWindowOverride, (__int64)pDirtyRegion, 
		Present_Jump);
#endif

	//return S_OK;
}


void nhook3::dump_buffer(IDirect3DDevice9 *Device)
{
	IDirect3DSurface9* pRenderTarget = NULL;
	IDirect3DSurface9* pDestTarget = NULL;
	const char file[] = "C:/Users/ÀçÁ¤/Desktop/Pickture.bmp";
	//const char file[] = "Pickture.bmp";
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
