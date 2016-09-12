// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <fstream>
//#include "../../../Common/Graphic/graphic.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <TlHelp32.h>
#include <thread>


#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "d3dx9.lib" )
#pragma comment( lib, "gdiplus.lib" ) 

using namespace std;
D3DDISPLAYMODE d3ddisplaymode;

struct handle_data {
	unsigned long process_id;
	HWND best_handle;
};


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


std::string wstr2str(const std::wstring &wstr)
{
	const int slength = (int)wstr.length() + 1;
	const int len = ::WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), slength, 0, 0, NULL, FALSE);
	char* buf = new char[len];
	::WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), slength, buf, len, NULL, FALSE);
	std::string r(buf);
	delete[] buf;
	return r;
}


BOOL is_main_window(HWND handle)
{
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !is_main_window(handle)) {
		return TRUE;
	}
	data.best_handle = handle;
	return FALSE;
}

HWND find_main_window(unsigned long process_id)
{
	handle_data data;
	data.process_id = process_id;
	data.best_handle = 0;
	EnumWindows(enum_windows_callback, (LPARAM)&data);
	return data.best_handle;
}


LRESULT CALLBACK TempWndProc(HWND   hwnd, UINT   uMsg, WPARAM wParam, LPARAM lParam )
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HRESULT WINAPI newEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	//Do your stuff here

	//Call the original (if you want)
	return pDevice->EndScene();
}

HWND InitWindow(HINSTANCE hInstance, const wstring &windowName, const RECT &windowRect, int nCmdShow, WNDPROC WndProc)
{
	const int X = windowRect.left;
	const int Y = windowRect.top;
	const int WIDTH = windowRect.right - windowRect.left;
	const int HEIGHT = windowRect.bottom - windowRect.top;

	//������ Ŭ���� ���� ����
	//���� �̷��� ������ ����ڴ� ��� ����
	WNDCLASS WndClass;
	WndClass.cbClsExtra = 0;			//�����쿡�� ����ϴ� ������ �޸𸮼���( �׳� 0 �̴�  �Ű澲������ )
	WndClass.cbWndExtra = 0;			//�����쿡�� ����ϴ� ������ �޸𸮼���( �׳� 0 �̴�  �Ű澲������ )
	WndClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);		//������ ������
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);			//�������� Ŀ����� ����
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);		//����������ܸ�� ����
	WndClass.hInstance = hInstance;				//���α׷��ν��Ͻ��ڵ� 
	WndClass.lpfnWndProc = (WNDPROC)WndProc;			//������ ���ν��� �Լ� ������
	WndClass.lpszMenuName = NULL;						//�޴��̸� ������ NULL
	WndClass.lpszClassName = windowName.c_str();				//���� �ۼ��ϰ� �ִ� ������ Ŭ������ �̸�
	WndClass.style = CS_HREDRAW | CS_VREDRAW;	//������ �׸��� ��� ���� ( ����� ����ɶ� ȭ�鰻�� CS_HREDRAW | CS_VREDRAW )

	//������ �ۼ��� ������ Ŭ�������� ���
	RegisterClass(&WndClass);

	//������ ����
	//������ ������ �ڵ��� �������� g_hWnd �� �޴´�.
	HWND hWnd = CreateWindow(
		windowName.c_str(),				//�����Ǵ� �������� Ŭ�����̸�
		windowName.c_str(),				//������ Ÿ��Ʋ�ٿ� ��µǴ� �̸�
		WS_OVERLAPPEDWINDOW,	//������ ��Ÿ�� WS_OVERLAPPEDWINDOW
		X,				//������ ���� ��ġ X 
		Y,				//������ ���� ��ġ Y
		WIDTH,				//������ ���� ũ�� ( �۾������� ũ�Ⱑ �ƴ� )
		HEIGHT,				//������ ���� ũ�� ( �۾������� ũ�Ⱑ �ƴ� )
		GetDesktopWindow(),		//�θ� ������ �ڵ� ( ���α׷����� �ֻ��� ������� NULL �Ǵ� GetDesktopWindow() )
		NULL,					//�޴� ID ( �ڽ��� ��Ʈ�� ��ü�� �������ΰ�� ��Ʈ�� ID �� ��	
		hInstance,				//�� �����찡 ���� ���α׷� �ν��Ͻ� �ڵ�
		NULL					//�߰� ���� NULL ( �Ű���� )
	);

	//�����츦 ��Ȯ�� �۾����� ũ��� �����
	RECT rcClient = { 0, 0, WIDTH, HEIGHT };
	AdjustWindowRect(&rcClient, WS_OVERLAPPEDWINDOW, FALSE);	//rcClient ũ�⸦ �۾� �������� �� ������ ũ�⸦ rcClient �� ���ԵǾ� ���´�.


	const int width = rcClient.right - rcClient.left;
	const int height = rcClient.bottom - rcClient.top;
	const int screenCX = GetSystemMetrics(SM_CXSCREEN);
	const int screenCY = GetSystemMetrics(SM_CYSCREEN);
	const int x = screenCX / 2 - width / 2;
	const int y = screenCY / 2 - height / 2;


	//������ ũ��� ������ ��ġ�� �ٲپ��ش�.
	SetWindowPos(hWnd, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);

	MoveWindow(hWnd, x, y, width, height, TRUE);

	ShowWindow(hWnd, nCmdShow);

	return hWnd;
}


void Test()
{
	ofstream ofs("dllinject_test.txt");

	LPDIRECT3DDEVICE9 ppReturnedDeviceInterface = NULL;

	//Just some typedefs:
	typedef HRESULT(WINAPI* oEndScene) (LPDIRECT3DDEVICE9 D3DDevice);
	static oEndScene EndScene;

	//Do this in a function or whatever
	HMODULE hDLL = GetModuleHandleA("d3d9");
	LPDIRECT3D9(__stdcall*pDirect3DCreate9)(UINT) = (LPDIRECT3D9(__stdcall*)(UINT))GetProcAddress(hDLL, "Direct3DCreate9");

	LPDIRECT3D9 pD3D = pDirect3DCreate9(D3D_SDK_VERSION);

	D3DDISPLAYMODE d3ddm;
	HRESULT hRes = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = d3ddm.Format;

// 	WNDCLASSEXA wc = { sizeof(WNDCLASSEXA),CS_CLASSDC,TempWndProc,0L,0L,GetModuleHandle(NULL),NULL,NULL,NULL,NULL,("1"),NULL };
// 	RegisterClassExA(&wc);
// 	HWND hWnd = CreateWindowA(("1"), NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), NULL, wc.hInstance, NULL);

	RECT r = { 0,0,400,400 };
	HWND hWnd = InitWindow(GetModuleHandle(NULL), L"test", r, SW_SHOW, TempWndProc);

	ofs << "pD3D = " << pD3D << endl;
	ofs << "hWnd = " << hWnd << endl;
	ofs.close();

// 	MSG msg;
// 	while (1)
// 	{
// 		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
// 		{
// 			TranslateMessage(&msg);
// 			DispatchMessage(&msg);
// 		}
// 	}
	

// 	hRes = pD3D->CreateDevice(
// 		D3DADAPTER_DEFAULT,
// 		D3DDEVTYPE_HAL,
// 		hWnd,
// 		D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
// 		&d3dpp, &ppReturnedDeviceInterface);
// 
// 	pD3D->Release();
// 	//DestroyWindow(hWnd);
// 	//ShowWindow(hWnd, SW_SHOW);
// 
// 	if (pD3D == NULL) {
// 		//printf ("WARNING: D3D FAILED");
// 		return false;
// 	}
// 
// 	if (!ppReturnedDeviceInterface)
// 	{
// 		ofs << "faile create device inteface" << endl;
// 	}
	
// 	pInterface = (unsigned long*)*((unsigned long*)ppReturnedDeviceInterface);
// 	EndScene = (oEndScene)(DWORD)pInterface[42];
// 	DetourTransactionBegin();
// 	DetourUpdateThread(GetCurrentThread());
// 	DetourAttach(&(PVOID&)EndScene, newEndScene);
// 	DetourTransactionCommit();
//	return true;
}


bool Test2()
{
	ofstream ofs("dllinject_test.txt");

	LPDIRECT3D9 d3d9;
	d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	wchar_t szFileName[MAX_PATH];
	GetModuleFileName(NULL, szFileName, MAX_PATH);
	ofs << "filename = " << wstr2str(szFileName) << endl;
	HWND hWnd = find_main_window(GetCurrentProcessId());
	ofs << "hwnd = " << hWnd << endl;
	char windowName[256];
	GetWindowTextA(hWnd, windowName, sizeof(windowName));
	ofs << "window name = " << windowName << endl;

	LPDIRECT3DDEVICE9 pDev;

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;

	if (FAILED(d3d9->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &pDev)))
	{
		//����̽� ���� ����
		d3d9->Release(); // Deivce �� ����� ���� ������ Direct3D9 ��ü�� ����
		d3d9 = NULL;

		//MessageBoxA(hWnd, "CreateDevice() - FAILED", "FAILED", MB_OK);
		ofs << "CreateDevice() - FAILED" << endl;
		ofs << "GetLastError() = " << GetLastError() << endl;
		return false;
	}


	//����̽� ���� ����
	d3d9->Release(); // Deivce �� ��������� �� ���̻� �ʿ���� ( �������! )
	d3d9 = NULL;
	return true;
}



bool InitDirectX(OUT LPDIRECT3DDEVICE9 &pDevice)
{
	ofstream ofs("dllinject_test.txt");
	
	LPDIRECT3D9 d3d9;
	d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	// �ϵ���� ������ ������ �ͼ� �ڽ��� ���� ���μ��� Ÿ���� ������
	D3DCAPS9 caps;

	//Direct3D9 ��ü ���� ���� ī���� �ϵ���� ������ ������ �´�.
	d3d9->GetDeviceCaps(
		D3DADAPTER_DEFAULT,			//�� ���÷��� �׷��� ī�� �׳� D3DADAPTER_DEFAULT
		D3DDEVTYPE_HAL,				//����̽�Ÿ�� ���� �׳� D3DDEVTYPE_HAL
		&caps						//����̽� ������ �޾ƿ� D3DCAPS9 ������
	);

	//������� ó������� ������ �÷��� ��
	int vertexProcessing = 0;

	//���� ��ġ�� ���� ���� �ϵ���� ����� �����Ѱ�
	if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddisplaymode);

	stringstream ss;
	ofs << d3ddisplaymode.Width << ", " << d3ddisplaymode.Height << ", " << d3ddisplaymode.Format << endl;
	
	wchar_t szFileName[MAX_PATH];
	GetModuleFileName(NULL, szFileName, MAX_PATH);
	ofs << "filename = " << wstr2str(szFileName) << endl;
	HWND hWnd = find_main_window(GetCurrentProcessId());
	ofs << "hwnd = " << hWnd << endl;
	char windowName[256];
	GetWindowTextA(hWnd, windowName, sizeof(windowName));
	ofs << "window name = " << windowName << endl;

	
	//3. D3DPRESENT_PARAMETERS ����ü ������ ����
	//���� �̷��� Device �� ����ٴٶ�� ����

	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;				//���� ���ۿ� ���׽� ���� ũ�� ���� 24bit ���ٽ� ���� 8 ��Ʈ
	d3dpp.BackBufferCount = 1;						//����� ���� �׳� 1��
	d3dpp.BackBufferFormat = d3ddisplaymode.Format;
	d3dpp.BackBufferHeight = d3ddisplaymode.Height;
	d3dpp.BackBufferWidth = d3ddisplaymode.Width;
	d3dpp.EnableAutoDepthStencil = true;						//�ڵ� ���̹��� ��� ���� ( �׳� true )
	d3dpp.Flags = 0;						//�߱� �÷��� ( �ϴ� 0 )
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;	//ȭ�� �ֻ��� ( �׳� D3DPRESENT_RATE_DEFAULT ����� �ֻ����� ���Ͻ� )
	d3dpp.hDeviceWindow = hWnd;					//Device �� ��µ� ������ �ڵ�
	d3dpp.MultiSampleQuality = 0;						//��Ƽ ���ø� ��
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;		//��Ƽ ���ø� Ÿ�� 
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;	//ȭ�� ���� ���� ( �׳� D3DPRESENT_INTERVAL_ONE ����� �ֻ����� ���Ͻ� )
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;	//ȭ�� ���� ü�� ���
	d3dpp.Windowed = false;						//������ ����? ( �̰� false �� Ǯ��ũ�� �ȴ�! )

	if (FAILED(d3d9->CreateDevice(
		D3DADAPTER_DEFAULT,					//�� ���÷��� �׷��� ī�� �׳� D3DADAPTER_DEFAULT
		D3DDEVTYPE_HAL,						//����̽�Ÿ�� ���� �׳� D3DDEVTYPE_HAL
		hWnd,										//����̽��� ����� ������ �ڵ�
		vertexProcessing,					//���� ó�� ��Ŀ� ���� �÷���
		&d3dpp,								//�տ��� ������ D3DPRESENT_PARAMETERS ����ü ������
		&pDevice							//���� ����̽� ����������
	)))
	{
		//����̽� ���� ����
		d3d9->Release(); // Deivce �� ����� ���� ������ Direct3D9 ��ü�� ����
		d3d9 = NULL;

		//MessageBoxA(hWnd, "CreateDevice() - FAILED", "FAILED", MB_OK);
		ofs << "CreateDevice() - FAILED" << endl;
		ofs << "GetLastError() = " << GetLastError() << endl;
		return false;
	}


	//����̽� ���� ����
	d3d9->Release(); // Deivce �� ��������� �� ���̻� �ʿ���� ( �������! )
	d3d9 = NULL;
	return true;
}


void dump_buffer()
{
	IDirect3DSurface9* pRenderTarget = NULL;
	IDirect3DSurface9* pDestTarget = NULL;
	const wstring file = L"Pickture.bmp";
	// sanity checks.

	LPDIRECT3DDEVICE9 Device = NULL;
	InitDirectX(Device);

	if (Device == NULL)
		return;

	// get the render target surface.
	HRESULT hr = Device->GetRenderTarget(0, &pRenderTarget);

	// create a destination surface.
	hr = Device->CreateOffscreenPlainSurface(d3ddisplaymode.Width,
		d3ddisplaymode.Height,
		d3ddisplaymode.Format,
		D3DPOOL_SYSTEMMEM,
		&pDestTarget,
		NULL);
	//copy the render target to the destination surface.
	hr = Device->GetRenderTargetData(pRenderTarget, pDestTarget);
	//save its contents to a bitmap file.
	hr = D3DXSaveSurfaceToFile(file.c_str(),
		D3DXIFF_BMP,
		pDestTarget,
		NULL,
		NULL);

	// clean up.
	pRenderTarget->Release();
	pDestTarget->Release();
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//LPDIRECT3DDEVICE9 dev;
		//InitDirectX(dev);
		//MessageBox(nullptr, L"injection success", L"dll injection", MB_OK);
		//InitDirectX();
		//std::thread th(Test, 0);
		// 		th.join();
		Test2();
		break;
	
	case DLL_THREAD_ATTACH:
	{
	}
	break;

	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
