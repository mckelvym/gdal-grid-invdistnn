// grf_ecw.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#define GRF_EXPORTS
#include "grf_ecw.h"

#if !defined( GET_MODULE_HANDLE_EX_FLAG_PIN )
#define GET_MODULE_HANDLE_EX_FLAG_PIN (0x00000001)
#endif

#ifdef WIN32
typedef BOOL ( WINAPI *GetModuleHandleExFunctionType )( DWORD, LPCTSTR, HMODULE* );
HMODULE localDLLHandle = 0;

BOOL LockDLLInMemory( HMODULE hModule )
{
  if ( localDLLHandle != 0 ) {
    return TRUE;
  }
  HMODULE kernel32Handle = GetModuleHandle( "kernel32.dll" );
  GetModuleHandleExFunctionType function;
  if ( kernel32Handle ) {
    function = ( GetModuleHandleExFunctionType )GetProcAddress( kernel32Handle, "GetModuleHandleExA" );
  }
  TCHAR moduleFilename[1024];
  if( GetModuleFileName( hModule, moduleFilename, 1024 ) == 0 ) {
    return FALSE;
  }
  localDLLHandle = LoadLibrary( moduleFilename );
  if ( localDLLHandle ) {
    if ( function ) {
      HMODULE anotherHandle = 0;
      if ( !( *function )( GET_MODULE_HANDLE_EX_FLAG_PIN, moduleFilename, &anotherHandle ) ) {
        // Failed, keep handle open.
        return FALSE;
      }
      FreeLibrary( localDLLHandle );
      FreeLibrary( anotherHandle );
    }
    // No GetModuleHandleEx function, keep handle open (leak it).
  }
  return TRUE;
}

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	switch ( fdwReason )
	{
	  case DLL_PROCESS_ATTACH:
      LockDLLInMemory( hinstDLL );
	  case DLL_THREAD_ATTACH:
	  case DLL_THREAD_DETACH:
	  case DLL_PROCESS_DETACH:
		  break;
	}

  return TRUE;
}
#endif

void GRF_API Register_ECW()
{
  GDALRegister_ECW();
}

void GRF_API Deregister_ECW(void* pDriver)
{
  GDALDeregister_ECW((GDALDriver*)pDriver);
}
