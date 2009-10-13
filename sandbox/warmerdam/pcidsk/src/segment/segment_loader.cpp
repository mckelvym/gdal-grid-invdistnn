/******************************************************************************
 *
 * Purpose: Load a shared library/DLL containing a segment type
 * 
 ******************************************************************************
 * Copyright (c) 2009
 * PCI Geomatics, 50 West Wilmot Street, Richmond Hill, Ont, Canada
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "pcidsk_config.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "pcidsk_exception.h"

#include "segment/segmentfactory.h"
#include "segment/pcidsksegmentbuilder.h"

#include "segment/uuid.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
#define CLOSE_LIBRARY(x) FreeLibrary(x)
#else
#define CLOSE_LIBRARY(x) dlclose(x)
#endif

using namespace PCIDSK;

typedef IPCIDSKSegmentBuilder *(*GetSegmentBuilder)(void);
typedef priv::PCIDSKUUID *(*GetBuilderUUID)(void);
typedef void (*GetPCIDSKLibraryVersion)(unsigned int *pnVerMajor, unsigned int *pnVerMinor);

/**
 * Attempt to register the DLL/shared object in sLibraryName.
 * @param sLibraryName path to the DLL
 * @return true if the registration is successful,
 *         false otherwise.
 */
bool priv::LoadSegmentLibrary(const std::string &sLibraryName)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
    HINSTANCE hLib = LoadLibrary(sLibraryName.c_str());
#else
    void *hLib = dlopen(sLibraryName.c_str(), RTLD_NOW);
#endif

    if (hLib == NULL)
    {
        return false;
    }

    // Check the PCIDSK Library Symbol Version
#if defined(_MSC_VER) || defined(__MINGW32__)
    GetPCIDSKLibraryVersion pVersion = (GetPCIDSKLibraryVersion)GetProcAddress(hLib, "GetPCIDSKLibraryVersion");
#else
    GetPCIDSKLibraryVersion pVersion = (GetPCIDSKLibraryVersion)dlsym(hLib, "GetPCIDSKLibraryVersion");
#endif

    if (pVersion == NULL)
    {
        CLOSE_LIBRARY(hLib);
        return false;
    }

    // Get the libray versions built against.
    unsigned int nVersionMajor = 0;
    unsigned int nVersionMinor = 0;

    pVersion(&nVersionMajor, &nVersionMinor);

    if (nVersionMajor != PCIDSK_SDK_MAJOR_VERSION)
    {
        CLOSE_LIBRARY(hLib);
        return false;
    }

#if defined(_MSC_VER) || defined(__MINGW32__)
    GetSegmentBuilder pGetBuilder = (GetSegmentBuilder)GetProcAddress(hLib, "GetSegmentBuilder");
#else
    GetSegmentBuilder pGetBuilder = (GetSegmentBuilder)dlsym(hLib, "GetSegmentBuilder");
#endif

    // Failed to get the builder function pointer?
    if (pGetBuilder == NULL)
    {
        CLOSE_LIBRARY(hLib);
        return false;
    }

    // Get the builder instance
    IPCIDSKSegmentBuilder *poBuilderInstance = pGetBuilder();

    if (poBuilderInstance == NULL) {
        CLOSE_LIBRARY(hLib);
        return false;
    }

    priv::CPCIDSKSegmentFactory *poFactory = priv::CPCIDSKSegmentFactory::GetInstance();
    poFactory->RegisterSegmentBuilderInstance(poBuilderInstance);

    return true;
}
