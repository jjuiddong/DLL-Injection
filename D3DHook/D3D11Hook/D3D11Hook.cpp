// D3D11Hook.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <d3d11.h>
#include <D3DX11tex.h>
#include <mmsystem.h>
#include "BeaEngine/headers/BeaEngine.h"
#include <stdio.h>
#include <fstream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "winmm.lib")

#ifdef _M_IX86
	#pragma comment(lib, "BeaEngine/Win32/Dll/BeaEngine.lib")
#elif _M_X64
	#pragma comment(lib, "BeaEngine/Win64/Dll/BeaEngine64.lib")
#endif


namespace d3dhook
{
	typedef HRESULT(__stdcall *D3D11PresentHook) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	HRESULT __stdcall hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
	const void* DetourFunc64(BYTE* const src, const BYTE* dest, const unsigned int jumplength);
	const void *DetourFunc86(BYTE* const src, const BYTE *dest, const unsigned int jumplength);
	void dump_buffer(IDXGISwapChain *pSwapChain);
	const unsigned int DisasmLengthCheck(const SIZE_T address, const unsigned int jumplength);
	BOOL CALLBACK EnumWindowsProcMy(HWND hwnd, LPARAM lParam);
	void Log(const char* fmt, ...);
	void RemoveLog();

	struct HookContext
	{
		BYTE original_code[64];
		SIZE_T dst_ptr;
		BYTE far_jmp[6];
	};

	HWND g_hWnd = NULL;
	ID3D11Device *g_pDevice = NULL;
	ID3D11DeviceContext *g_pContext = NULL;
	ID3D11Texture2D *g_texture_to_save = NULL;
	ID3D11Texture2D *g_texture_to_convert = NULL;

	D3D11PresentHook g_phookD3D11Present = NULL;
	HookContext* g_presenthook64 = NULL;
	void* g_detourBuffer[3];
	char g_saveFileName[256];
}

using namespace d3dhook;


DWORD __stdcall hook(LPVOID hModule)
{
	RemoveLog();
	Log("hook initialize \n");

	memset(g_detourBuffer, 0, sizeof(g_detourBuffer) * sizeof(void*));
	EnumWindows(EnumWindowsProcMy, GetCurrentProcessId()); // find HWND

	GetCurrentDirectoryA(sizeof(g_saveFileName), g_saveFileName); // save image file name
	strcat_s(g_saveFileName, "\\backbuffer.bmp");

	Log("save image file path = %s \n", g_saveFileName);

	char titleStr[128] = { 0 };
	GetWindowTextA(g_hWnd, titleStr, sizeof(titleStr));
	Log("ProcessId = %d, HWND = %x, title = %s \n", GetCurrentProcessId(), g_hWnd, titleStr);

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = g_hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;//((GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP) != 0) ? FALSE : TRUE;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	IDXGISwapChain* pSwapChain;
	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &featureLevel, 1
		, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &g_pDevice, NULL, &g_pContext)))
	{
		MessageBoxA(g_hWnd, "Failed to create directX device and swapchain!", "error", MB_ICONERROR);
		return NULL;
	}

	DWORD_PTR* pSwapChainVtable = NULL;
	DWORD_PTR* pDeviceContextVTable = NULL;

	pSwapChainVtable = (DWORD_PTR*)pSwapChain;
	pSwapChainVtable = (DWORD_PTR*)pSwapChainVtable[0];

	pDeviceContextVTable = (DWORD_PTR*)g_pContext;
	pDeviceContextVTable = (DWORD_PTR*)pDeviceContextVTable[0];

#ifdef _M_X64
	g_phookD3D11Present = (D3D11PresentHook)DetourFunc64((BYTE*)pSwapChainVtable[8], (BYTE*)hookD3D11Present, 16);

#else
	g_phookD3D11Present = (D3D11PresentHook)DetourFunc86((BYTE*)pSwapChainVtable[8], (BYTE*)hookD3D11Present, 16);

	DWORD dwOld;
	VirtualProtect(g_phookD3D11Present, 2, PAGE_EXECUTE_READWRITE, &dwOld);
#endif

	g_pDevice->Release();
	g_pContext->Release();
	pSwapChain->Release();

	return true;
}


void clearhook()
{
	for (int i = 0; i < sizeof(g_detourBuffer) / sizeof(void*); ++i)
	{
		if (g_detourBuffer[i])
		{
#ifdef _WIN64
			VirtualFree(g_detourBuffer[i], 0, MEM_RELEASE);
#else
			delete[] g_detourBuffer[i];
#endif
		}
	}
}


#ifdef _M_X64
	const unsigned int d3dhook::DisasmLengthCheck(const SIZE_T address, const unsigned int jumplength)
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

	const void* d3dhook::DetourFunc64(BYTE* const src, const BYTE* dest, const unsigned int jumplength)
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

		Log("DetourFunc64 begin \n");

		// Save original code and apply detour. The detour code is:
		BYTE detour[] = {
			0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,  // mov rax, 0xCCCCCCCCCCCCCCCC
			0xFF, 0xE0 // jmp rax
		};

		const unsigned int length = DisasmLengthCheck((SIZE_T)src, jumplength);
		memcpy(g_presenthook64->original_code, src, length);
		memcpy(&g_presenthook64->original_code[length], detour, sizeof(detour));
		*(SIZE_T*)&g_presenthook64->original_code[length + 2] = (SIZE_T)src + length;

		Log("length = %d, return address = %I64x \n", length, (SIZE_T)src + length);

		// Build a far jump to the destination function.
		*(WORD*)&g_presenthook64->far_jmp = 0x25FF;
		*(DWORD*)(g_presenthook64->far_jmp + 2) = (DWORD)((SIZE_T)g_presenthook64 - (SIZE_T)src + FIELD_OFFSET(HookContext, dst_ptr) - 6);
		g_presenthook64->dst_ptr = (SIZE_T)dest;

		Log("jump address = %I64x \n", (DWORD)((SIZE_T)g_presenthook64 - (SIZE_T)src + FIELD_OFFSET(HookContext, dst_ptr) - 6));

		// Write the hook to the original function.
		DWORD flOld = 0;
		VirtualProtect(src, 6, PAGE_EXECUTE_READWRITE, &flOld);
		memcpy(src, g_presenthook64->far_jmp, sizeof(g_presenthook64->far_jmp));
		VirtualProtect(src, 6, flOld, &flOld);

		// Return a pointer to the original code.
		return g_presenthook64->original_code;
	}

#else

	const void *d3dhook::DetourFunc86(BYTE* const src, const BYTE *dest, const unsigned int jumplength)
	{
		BYTE *jmp = (BYTE*)malloc(jumplength + 5);

		for (int i = 0; i < sizeof(g_detourBuffer) / sizeof(void*); ++i)
		{
			if (!g_detourBuffer[i])
			{
				g_detourBuffer[i] = jmp;
				break;
			}
		}

		DWORD dwBack;
		VirtualProtect(src, jumplength, PAGE_EXECUTE_READWRITE, &dwBack);
		memcpy(jmp, src, jumplength);
		jmp += jumplength;
		jmp[0] = 0xE9;
		*(DWORD*)(jmp + 1) = (DWORD)(src + jumplength - jmp) - 5;
		src[0] = 0xE9;
		*(DWORD*)(src + 1) = (DWORD)(dest - src) - 5;
		for (unsigned int i = 5; i < jumplength; i++)  
			src[i] = 0x90;

		VirtualProtect(src, jumplength, dwBack, &dwBack);
		return (jmp - jumplength);
	}

#endif // #ifdef _M_X64


HRESULT  __stdcall  d3dhook::hookD3D11Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool firstTime = true;
	if (firstTime)
	{
		Log("present hook11() getdevice \n");

		pSwapChain->GetDevice(__uuidof(g_pDevice), (void**)&g_pDevice);
		g_pDevice->GetImmediateContext(&g_pContext);
		firstTime = false;
	}

	// 20 fps
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


void d3dhook::dump_buffer(IDXGISwapChain *pSwapChain)
{
	ID3D11Texture2D* pBuffer = NULL;
	//const char filename[] = "C:/Users/ÀçÁ¤/Desktop/Pickture.bmp";

	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBuffer);

	if (g_texture_to_save == NULL)
	{
		D3D11_TEXTURE2D_DESC td;
		pBuffer->GetDesc(&td);
		g_pDevice->CreateTexture2D(&td, NULL, &g_texture_to_save);
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		g_pDevice->CreateTexture2D(&td, NULL, &g_texture_to_convert);
		Log("texture format = %d \n", td.Format);
	}

	g_pContext->CopyResource(g_texture_to_save, pBuffer);

	_D3DX11_TEXTURE_LOAD_INFO load;
	D3DX11LoadTextureFromTexture(g_pContext, g_texture_to_save, &load, g_texture_to_convert);
	D3DX11SaveTextureToFileA(g_pContext, g_texture_to_convert, D3DX11_IFF_BMP, g_saveFileName);

	pBuffer->Release();
}


void d3dhook::Log(const char* fmt, ...)
{
	char textString[256];
	ZeroMemory(textString, sizeof(textString));

	va_list args;
	va_start(args, fmt);
	vsnprintf_s(textString, sizeof(textString) - 1, _TRUNCATE, fmt, args);
	va_end(args);

	std::ofstream ofs("log.txt", std::ios::app);
	if (ofs.is_open())
		ofs << textString;
}


void d3dhook::RemoveLog()
{
	FILE *fp = fopen("log.txt", "w");
	if (fp)
	{
		fputs("", fp);
		fclose(fp);
	}
}


BOOL CALLBACK d3dhook::EnumWindowsProcMy(HWND hwnd, LPARAM lParam)
{
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	if (lpdwProcessId == lParam)
	{
		g_hWnd = hwnd;
		return FALSE;
	}
	return TRUE;
}
