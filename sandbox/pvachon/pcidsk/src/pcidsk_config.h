/******************************************************************************
 *
 * Purpose:  Primary include file for PCIDSK SDK.
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

#ifndef PCIDSK_CONFIG_H_INCLUDED
#define PCIDSK_CONFIG_H_INCLUDED

namespace PCIDSK {

    typedef unsigned char uint8;
    typedef int           int32;
    typedef unsigned int  uint32;
    
#if defined(_MSC_VER)  
    typedef __int64          int64;
    typedef unsigned __int64 uint64;
#else
    typedef long long          int64;
    typedef unsigned long long uint64;
#endif

#ifndef PCIDSK_DLL
#if defined(_MSC_VER) 
#  define PCIDSK_DLL     __declspec(dllexport)
#else
#  define PCIDSK_DLL
#endif
#endif

};

// #define MISSING_VSNPRINTF

#endif // PCIDSK_CONFIG_H_INCLUDED



