// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <TlHelp32.h>
#include "hook1.h"
#include "hook2.h"
#include "hook3.h"

// https://github.com/thereals0beit/D3DHook

 
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hook3();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

