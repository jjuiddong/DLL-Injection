
#include "stdafx.h"
#include "hook11_64.h"
#include <d3d11.h>
#include <D3DX11tex.h>
#include <mmsystem.h>
#include "BeaEngine/headers/BeaEngine.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "BeaEngine/Win64/Dll/BeaEngine64.lib")


namespace nhook11
{
	HRESULT __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
	void *DetourCreate(BYTE *src, const BYTE *dst, const int len);
	const void* DetourFunc64(BYTE* const src, const BYTE* dest, const unsigned int jumplength);
	void dump_buffer(IDXGISwapChain *pSwapChain);
	const unsigned int DisasmLengthCheck(const SIZE_T address, const unsigned int jumplength);


	typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
	typedef void(__stdcall *D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
	typedef void(__stdcall *D3D11ClearRenderTargetViewHook) (ID3D11DeviceContext* pContext, ID3D11RenderTargetView *pRenderTargetView, const FLOAT ColorRGBA[4]);

	ID3D11Device *g_pDevice = NULL;
	ID3D11DeviceContext *g_pContext = NULL;
	ID3D11Texture2D *g_texture_to_save = NULL;
	ID3D11Texture2D *g_texture_to_convert = NULL;

	D3D11PresentHook g_phookD3D11Present = NULL;
	D3D11DrawIndexedHook g_phookD3D11DrawIndexed = NULL;
	D3D11ClearRenderTargetViewHook g_phookD3D11ClearRenderTargetView = NULL;
	__int64 Present_Jump;

	struct HookContext
	{
		BYTE original_code[64];
		SIZE_T dst_ptr;
		BYTE far_jmp[6];
	};

	HookContext* g_presenthook64 = NULL;
	void* g_detourBuffer[3];
}

using namespace nhook11;


DWORD __stdcall hook11(LPVOID)
{
	dbg::RemoveLog();
	dbg::Log("hook11 \n");

	memset(g_detourBuffer, 0, sizeof(g_detourBuffer) * sizeof(void*));

	DWORD_PTR* pSwapChainVtable = NULL;
	DWORD_PTR* pDeviceContextVTable = NULL;

	//HWND hWnd = FindWindowA(NULL, "¿ö ½ã´õ");
	HWND hWnd = FindWindowA(NULL, "Grand Theft Auto V");
	//HWND hWnd = FindWindowA(NULL, "Direct3D 11 Tutorial 6");
	//HWND hWnd = FindWindowA(NULL, "SubD11");
	//HWND hWnd = FindWindowA(NULL, "MultithreadedRendering11");	
	//HWND hWnd = FindWindowA(NULL, "VarianceShadows11");	
	//HWND hWnd = FindWindowA(NULL, "DiRT 3");
	//HWND hWnd = FindWindowA(NULL, "Project CARS¢â");
	IDXGISwapChain* pSwapChain;

	char titleStr[128] = { 0 };
	GetWindowTextA(hWnd, titleStr, sizeof(titleStr));
	dbg::Log("HWND = %x, name = %s \n", hWnd, titleStr);

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;//((GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP) != 0) ? FALSE : TRUE;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &featureLevel, 1
		, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &g_pDevice, NULL, &g_pContext)))
	{
		MessageBoxA(hWnd, "Failed to create directX device and swapchain!", "uBoos?", MB_ICONERROR);
		return NULL;
	}

	pSwapChainVtable = (DWORD_PTR*)pSwapChain;
	pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];

	pDeviceContextVTable = (DWORD_PTR*)g_pContext;
	pDeviceContextVTable = (DWORD_PTR*)pDeviceContextVTable[0];

	Present_Jump = (pSwapChainVtable[8] + 15);
	//g_phookD3D11Present = (D3D11PresentHook)DetourCreate((BYTE*)pSwapChainVtable[8], (BYTE*)hookD3D11Present, 15);
	g_phookD3D11Present = (D3D11PresentHook)DetourFunc64((BYTE*)pSwapChainVtable[8], (BYTE*)hookD3D11Present, 16);
	//phookD3D11DrawIndexed = (D3D11DrawIndexedHook)DetourFunc64((BYTE*)pDeviceContextVTable[12], (BYTE*)hookD3D11DrawIndexed, 16);
	//phookD3D11ClearRenderTargetView = (D3D11ClearRenderTargetViewHook)DetourFunc64((BYTE*)pDeviceContextVTable[50], (BYTE*)hookD3D11ClearRenderTargetView, 16);
	//DWORD dwOld;
	//VirtualProtect(g_phookD3D11Present, 2, PAGE_EXECUTE_READWRITE, &dwOld);

	g_pDevice->Release();
	g_pContext->Release();
	pSwapChain->Release();

	dbg::Log("hook11 end \n");
	return true;
}


// x64 absolute address jump
void *nhook11::DetourCreate(BYTE *src, const BYTE *dst, const int len)
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



const unsigned int nhook11::DisasmLengthCheck(const SIZE_T address, const unsigned int jumplength)
{
	DISASM disasm;
	memset(&disasm, 0, sizeof(DISASM));

	disasm.EIP = (UIntPtr)address;
	disasm.Archi = 0x40;

	unsigned int processed = 0;
	while (processed < jumplength)
	{
		const int len = Disasm(&disasm);
		if (len == UNKNOWN_OPCODE)
		{
			++disasm.EIP;
		}
		else
		{
			processed += len;
			disasm.EIP += len;
		}
	}

	return processed;
}

const void* nhook11::DetourFunc64(BYTE* const src, const BYTE* dest, const unsigned int jumplength)
{
	// Allocate a memory page that is going to contain executable code.
	MEMORY_BASIC_INFORMATION mbi;
	for (SIZE_T addr = (SIZE_T)src; addr > (SIZE_T)src - 0x80000000; addr = (SIZE_T)mbi.BaseAddress - 1)
	{
		if (!VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi)))
		{
			break;
		}

		if (mbi.State == MEM_FREE)
		{
			if (g_presenthook64 = (HookContext*)VirtualAlloc(mbi.BaseAddress, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE))
			{
				break;
			}
		}
	}

	// If allocating a memory page failed, the function failed.
	if (!g_presenthook64)
	{
		return NULL;
	}

	// Select a pointer slot for the memory page to be freed on unload.
	for (int i = 0; i < sizeof(g_detourBuffer) / sizeof(void*); ++i)
	{
		if (!g_detourBuffer[i])
		{
			g_detourBuffer[i] = g_presenthook64;
			break;
		}
	}

	dbg::Log("DetourFunc64 begin \n");

	// Save original code and apply detour. The detour code is:
	//BYTE detour[] = { 
	//	0x50, // push rax
	//	0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,  // movabs rax, 0xCCCCCCCCCCCCCCCC
	//	0x48, 0x87, 0x04, 0x24, // xchg rax, [rsp]
	//	0xC3 }; // ret
	BYTE detour[] = {
		0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,  // movabs rax, 0xCCCCCCCCCCCCCCCC
		0xFF, 0xE0 // jmp rax
		};

	const unsigned int length = DisasmLengthCheck((SIZE_T)src, jumplength);
	memcpy(g_presenthook64->original_code, src, length);
	memcpy(&g_presenthook64->original_code[length], detour, sizeof(detour));
	*(SIZE_T*)&g_presenthook64->original_code[length + 2] = (SIZE_T)src + length;

	dbg::Log("length = %d, return address = %I64x \n", length, (SIZE_T)src + length);

	// Build a far jump to the destination function.
	*(WORD*)&g_presenthook64->far_jmp = 0x25FF;
	*(DWORD*)(g_presenthook64->far_jmp + 2) = (DWORD)((SIZE_T)g_presenthook64 - (SIZE_T)src + FIELD_OFFSET(HookContext, dst_ptr) - 6);
	g_presenthook64->dst_ptr = (SIZE_T)dest;

	dbg::Log("jump address = %I64x \n", (DWORD)((SIZE_T)g_presenthook64 - (SIZE_T)src + FIELD_OFFSET(HookContext, dst_ptr) - 6));

	// Write the hook to the original function.
	DWORD flOld = 0;
	VirtualProtect(src, 6, PAGE_EXECUTE_READWRITE, &flOld);
	memcpy(src, g_presenthook64->far_jmp, sizeof(g_presenthook64->far_jmp));
	VirtualProtect(src, 6, flOld, &flOld);

	// Return a pointer to the original code.
	return g_presenthook64->original_code;
}


#ifdef _WIN64
	extern "C" int Jump64(__int64 arg1, __int64 arg2, __int64 arg3, __int64 address);
#endif


HRESULT  __stdcall  nhook11::hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool firstTime = true;
	if (firstTime)
	{
		dbg::Log("present hook11() getdevice \n");

		pSwapChain->GetDevice(__uuidof(g_pDevice), (void**)&g_pDevice);
		g_pDevice->GetImmediateContext(&g_pContext);
		firstTime = false;
	}

	static int oldT = 0;
	const int curT = timeGetTime();
	const int deltaT = curT - oldT;
	if (deltaT > 50)
	{
		oldT = curT;
		dump_buffer(pSwapChain);
	}	

	return g_phookD3D11Present(pSwapChain, SyncInterval, Flags);
}


void nhook11::dump_buffer(IDXGISwapChain *pSwapChain)
{
	ID3D11Texture2D* pBuffer = NULL;
	const char filename[] = "C:/Users/ÀçÁ¤/Desktop/Pickture.bmp";
	HRESULT r1 = 0, r2 = 0, r3 = 0, r4 = 0;

	r1 = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBuffer);

	if (g_texture_to_save == NULL)
	{
		D3D11_TEXTURE2D_DESC td;
		pBuffer->GetDesc(&td);
		r2 = g_pDevice->CreateTexture2D(&td, NULL, &g_texture_to_save);
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		r3 = g_pDevice->CreateTexture2D(&td, NULL, &g_texture_to_convert);
		dbg::Log("texture format = %d \n", td.Format);
	}

	g_pContext->CopyResource(g_texture_to_save, pBuffer);

	_D3DX11_TEXTURE_LOAD_INFO load;
	D3DX11LoadTextureFromTexture(g_pContext, g_texture_to_save, &load, g_texture_to_convert);
	r4 = D3DX11SaveTextureToFileA(g_pContext, g_texture_to_convert, D3DX11_IFF_BMP, filename);

	//dbg::Log("dump buffer r1=%x, r2=%x, r3=%x \n", r1, r2, r3);

	pBuffer->Release();
}
