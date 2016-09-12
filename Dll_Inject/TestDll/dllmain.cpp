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

	//윈도우 클레스 정보 생성
	//내가 이러한 윈도를 만들겠다 라는 정보
	WNDCLASS WndClass;
	WndClass.cbClsExtra = 0;			//윈도우에서 사용하는 여분의 메모리설정( 그냥 0 이다  신경쓰지말자 )
	WndClass.cbWndExtra = 0;			//윈도우에서 사용하는 여분의 메모리설정( 그냥 0 이다  신경쓰지말자 )
	WndClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);		//윈도우 배경색상
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);			//윈도우의 커서모양 결정
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);		//윈도우아이콘모양 결정
	WndClass.hInstance = hInstance;				//프로그램인스턴스핸들 
	WndClass.lpfnWndProc = (WNDPROC)WndProc;			//윈도우 프로시져 함수 포인터
	WndClass.lpszMenuName = NULL;						//메뉴이름 없으면 NULL
	WndClass.lpszClassName = windowName.c_str();				//지금 작성하고 있는 윈도우 클레스의 이름
	WndClass.style = CS_HREDRAW | CS_VREDRAW;	//윈도우 그리기 방식 설정 ( 사이즈가 변경될때 화면갱신 CS_HREDRAW | CS_VREDRAW )

	//위에서 작성한 윈도우 클레스정보 등록
	RegisterClass(&WndClass);

	//윈도우 생성
	//생성된 윈도우 핸들을 전역변수 g_hWnd 가 받는다.
	HWND hWnd = CreateWindow(
		windowName.c_str(),				//생성되는 윈도우의 클래스이름
		windowName.c_str(),				//윈도우 타이틀바에 출력되는 이름
		WS_OVERLAPPEDWINDOW,	//윈도우 스타일 WS_OVERLAPPEDWINDOW
		X,				//윈도우 시작 위치 X 
		Y,				//윈도우 시작 위치 Y
		WIDTH,				//윈도우 가로 크기 ( 작업영역의 크기가 아님 )
		HEIGHT,				//윈도우 세로 크기 ( 작업영역의 크기가 아님 )
		GetDesktopWindow(),		//부모 윈도우 핸들 ( 프로그램에서 최상위 윈도우면 NULL 또는 GetDesktopWindow() )
		NULL,					//메뉴 ID ( 자신의 컨트롤 객체의 윈도우인경우 컨트롤 ID 가 된	
		hInstance,				//이 윈도우가 물릴 프로그램 인스턴스 핸들
		NULL					//추가 정보 NULL ( 신경끄자 )
	);

	//윈도우를 정확한 작업영역 크기로 맞춘다
	RECT rcClient = { 0, 0, WIDTH, HEIGHT };
	AdjustWindowRect(&rcClient, WS_OVERLAPPEDWINDOW, FALSE);	//rcClient 크기를 작업 영영으로 할 윈도우 크기를 rcClient 에 대입되어 나온다.


	const int width = rcClient.right - rcClient.left;
	const int height = rcClient.bottom - rcClient.top;
	const int screenCX = GetSystemMetrics(SM_CXSCREEN);
	const int screenCY = GetSystemMetrics(SM_CYSCREEN);
	const int x = screenCX / 2 - width / 2;
	const int y = screenCY / 2 - height / 2;


	//윈도우 크기와 윈도우 위치를 바꾸어준다.
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
		//디바이스 생성 실패
		d3d9->Release(); // Deivce 를 만들기 위해 생성된 Direct3D9 객체를 해제
		d3d9 = NULL;

		//MessageBoxA(hWnd, "CreateDevice() - FAILED", "FAILED", MB_OK);
		ofs << "CreateDevice() - FAILED" << endl;
		ofs << "GetLastError() = " << GetLastError() << endl;
		return false;
	}


	//디바이스 생성 성공
	d3d9->Release(); // Deivce 를 만들었으니 넌 더이상 필요없다 ( 사라져라! )
	d3d9 = NULL;
	return true;
}



bool InitDirectX(OUT LPDIRECT3DDEVICE9 &pDevice)
{
	ofstream ofs("dllinject_test.txt");
	
	LPDIRECT3D9 d3d9;
	d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	// 하드웨어 정보를 가지고 와서 자신의 정점 프로세스 타입을 정하자
	D3DCAPS9 caps;

	//Direct3D9 객체 통해 비디오 카드의 하드웨어 정보를 가지고 온다.
	d3d9->GetDeviceCaps(
		D3DADAPTER_DEFAULT,			//주 디스플레이 그래픽 카드 그냥 D3DADAPTER_DEFAULT
		D3DDEVTYPE_HAL,				//디바이스타입 설정 그냥 D3DDEVTYPE_HAL
		&caps						//디바이스 정보를 받아올 D3DCAPS9 포인터
	);

	//정점계산 처리방식을 지정할 플레그 값
	int vertexProcessing = 0;

	//정점 위치와 광원 계산시 하드웨어 사용이 가능한가
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

	
	//3. D3DPRESENT_PARAMETERS 구조체 정보를 생성
	//내가 이러한 Device 를 만들겟다라는 정보

	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;				//뎁스 버퍼와 스테실 버퍼 크기 뎁스 24bit 스텐실 버퍼 8 비트
	d3dpp.BackBufferCount = 1;						//백버퍼 갯수 그냥 1개
	d3dpp.BackBufferFormat = d3ddisplaymode.Format;
	d3dpp.BackBufferHeight = d3ddisplaymode.Height;
	d3dpp.BackBufferWidth = d3ddisplaymode.Width;
	d3dpp.EnableAutoDepthStencil = true;						//자동 깊이버퍼 사용 여부 ( 그냥 true )
	d3dpp.Flags = 0;						//추기 플래그 ( 일단 0 )
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;	//화면 주사율 ( 그냥 D3DPRESENT_RATE_DEFAULT 모니터 주사율과 동일시 )
	d3dpp.hDeviceWindow = hWnd;					//Device 가 출력될 윈도우 핸들
	d3dpp.MultiSampleQuality = 0;						//멀티 샘플링 질
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;		//멀티 샘플링 타입 
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;	//화면 전송 간격 ( 그냥 D3DPRESENT_INTERVAL_ONE 모니터 주사율과 동일시 )
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;	//화면 스왑 체인 방식
	d3dpp.Windowed = false;						//윈도우 모드냐? ( 이게 false 면 풀스크린 된다! )

	if (FAILED(d3d9->CreateDevice(
		D3DADAPTER_DEFAULT,					//주 디스플레이 그래픽 카드 그냥 D3DADAPTER_DEFAULT
		D3DDEVTYPE_HAL,						//디바이스타입 설정 그냥 D3DDEVTYPE_HAL
		hWnd,										//디바이스를 사용할 윈도우 핸들
		vertexProcessing,					//정점 처리 방식에 대한 플레그
		&d3dpp,								//앞에서 정의한 D3DPRESENT_PARAMETERS 구조체 포인터
		&pDevice							//얻어올 디바이스 더블포인터
	)))
	{
		//디바이스 생성 실패
		d3d9->Release(); // Deivce 를 만들기 위해 생성된 Direct3D9 객체를 해제
		d3d9 = NULL;

		//MessageBoxA(hWnd, "CreateDevice() - FAILED", "FAILED", MB_OK);
		ofs << "CreateDevice() - FAILED" << endl;
		ofs << "GetLastError() = " << GetLastError() << endl;
		return false;
	}


	//디바이스 생성 성공
	d3d9->Release(); // Deivce 를 만들었으니 넌 더이상 필요없다 ( 사라져라! )
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
