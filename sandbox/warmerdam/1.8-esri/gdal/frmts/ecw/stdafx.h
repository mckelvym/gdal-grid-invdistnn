#pragma once

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#endif
#include "gdal_priv.h"

extern void GDALRegister_ECW();
extern void GDALDeregister_ECW(GDALDriver* pDriver);
