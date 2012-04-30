// grf_ecw.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#define GRF_EXPORTS
#include "gdal_webp.h"

#ifdef WIN32

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	switch ( fdwReason )
	{
	  case DLL_PROCESS_ATTACH:
	  case DLL_THREAD_ATTACH:
	  case DLL_THREAD_DETACH:
	  case DLL_PROCESS_DETACH:
		  break;
	}

  return TRUE;
}
#endif
