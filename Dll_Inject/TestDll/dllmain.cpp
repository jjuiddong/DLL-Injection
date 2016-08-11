// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <fstream>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		MessageBox(nullptr, L"injection success", L"dll injection", MB_OK);
// 		std::ofstream ofs("test_aaa.txt");
// 		if (ofs.is_open())
// 		{
// 			ofs << "ok" << std::endl;
// 			ofs.close();
// 		}
	}
	break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

