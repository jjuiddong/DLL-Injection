
#include "stdafx.h"
#include "hook11.h"
#include <d3d11.h>
#include <D3DX11tex.h>
#include <mmsystem.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "winmm.lib")


namespace nhook11
{
	HRESULT __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
	typedef void(__stdcall *D3D11DrawIndexedHook) (ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
	typedef void(__stdcall *D3D11ClearRenderTargetViewHook) (ID3D11DeviceContext* pContext, ID3D11RenderTargetView *pRenderTargetView, const FLOAT ColorRGBA[4]);

	ID3D11Device *pDevice = NULL;
	ID3D11DeviceContext *pContext = NULL;
	ID3D11Texture2D *texture_to_save = NULL;
	ID3D11Texture2D *texture_to_convert = NULL;

	D3D11PresentHook phookD3D11Present = NULL;
	D3D11DrawIndexedHook phookD3D11DrawIndexed = NULL;
	D3D11ClearRenderTargetViewHook phookD3D11ClearRenderTargetView = NULL;

	void *DetourCreate(BYTE *src, const BYTE *dst, const int len);
	void dump_buffer(IDXGISwapChain *pSwapChain);
}

using namespace nhook11;


DWORD __stdcall hook11(LPVOID)
{
	dbg::RemoveLog();
	dbg::Log("hook11 \n");

	DWORD_PTR* pSwapChainVtable = NULL;
	DWORD_PTR* pDeviceContextVTable = NULL;

//	HWND hWnd = FindWindowA(NULL, "Direct3D 11 Tutorial 6");
	HWND hWnd = FindWindowA(NULL, "DiRT 3");
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
		, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &pDevice, NULL, &pContext)))
	{
		MessageBoxA(hWnd, "Failed to create directX device and swapchain!", "uBoos?", MB_ICONERROR);
		return NULL;
	}

	pSwapChainVtable = (DWORD_PTR*)pSwapChain;
	pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];

	pDeviceContextVTable = (DWORD_PTR*)pContext;
	pDeviceContextVTable = (DWORD_PTR*)pDeviceContextVTable[0];

	phookD3D11Present = (D3D11PresentHook)DetourCreate((BYTE*)pSwapChainVtable[8], (BYTE*)hookD3D11Present, 5);
	//phookD3D11DrawIndexed = (D3D11DrawIndexedHook)DetourFunc((BYTE*)pDeviceContextVTable[12], (BYTE*)hookD3D11DrawIndexed, 5);
	//phookD3D11ClearRenderTargetView = (D3D11ClearRenderTargetViewHook)DetourFunc((BYTE*)pDeviceContextVTable[50], (BYTE*)hookD3D11ClearRenderTargetView, 5);

	DWORD dwOld;
	VirtualProtect(phookD3D11Present, 2, PAGE_EXECUTE_READWRITE, &dwOld);

	pDevice->Release();
	pContext->Release();
	pSwapChain->Release();
	return true;
}

void *nhook11::DetourCreate(BYTE *src, const BYTE *dst, const int len)
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


HRESULT __stdcall nhook11::hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	//dbg::Log("present hook11() \n");

	static bool firstTime = true;
	if (firstTime)
	{
		dbg::Log("present hook11() getdevice \n");

		pSwapChain->GetDevice(__uuidof(pDevice), (void**)&pDevice);
		pDevice->GetImmediateContext(&pContext);
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

	return phookD3D11Present(pSwapChain, SyncInterval, Flags);
}


void nhook11::dump_buffer(IDXGISwapChain *pSwapChain)
{
	ID3D11Texture2D* pBuffer = NULL;
	const char filename[] = "C:/Users/ÀçÁ¤/Desktop/Pickture.bmp";
	HRESULT r1=0, r2=0, r3=0, r4=0;

	r1 = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBuffer);

	if (texture_to_save == NULL)
	{
		D3D11_TEXTURE2D_DESC td;
		pBuffer->GetDesc(&td);
		r2 = pDevice->CreateTexture2D(&td, NULL, &texture_to_save);
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		r3 = pDevice->CreateTexture2D(&td, NULL, &texture_to_convert);
		dbg::Log("texture format = %d \n", td.Format);
	}

	pContext->CopyResource(texture_to_save, pBuffer);

	_D3DX11_TEXTURE_LOAD_INFO load;
	D3DX11LoadTextureFromTexture(pContext, texture_to_save, &load, texture_to_convert);
	r4 = D3DX11SaveTextureToFileA(pContext, texture_to_convert, D3DX11_IFF_BMP, filename);

	//dbg::Log("dump buffer r1=%x, r2=%x, r3=%x \n", r1, r2, r3);

	pBuffer->Release();
}
