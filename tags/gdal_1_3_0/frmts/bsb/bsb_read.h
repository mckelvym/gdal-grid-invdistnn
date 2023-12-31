/******************************************************************************
 * $Id$
 *
 * Project:  BSB Reader
 * Purpose:  non-GDAL BSB API Declarations
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.5  2002/11/04 04:26:45  warmerda
 * preliminary work on write support
 *
 * Revision 1.4  2002/07/19 22:05:15  warmerda
 * added support for NO1 (encrypted) files
 *
 * Revision 1.3  2002/07/19 20:32:57  warmerda
 * added nVersion to structure.
 *
 * Revision 1.2  2001/12/08 21:58:32  warmerda
 * save header
 *
 * Revision 1.1  2001/12/08 04:35:16  warmerda
 * New
 *
 */

#ifndef _BSBREAD_H_INCLUDED
#define _BSBREAD_H_INCLUDED

#include "cpl_port.h"

CPL_C_START

typedef struct {
    FILE        *fp;

    int		nXSize;
    int         nYSize;

    int         nPCTSize;
    unsigned char *pabyPCT;

    char        **papszHeader;

    int		*panLineOffset;

    int         nColorSize;

    int		nVersion; /* times 100 */

    int         bNO1;

    int         bNewFile;
    int         nLastLineWritten;
} BSBInfo;

BSBInfo CPL_DLL *BSBOpen( const char *pszFilename );
int CPL_DLL BSBReadScanline( BSBInfo *psInfo, int nScanline, 
                             unsigned char *pabyScanlineBuf );
void CPL_DLL BSBClose( BSBInfo *psInfo );

BSBInfo CPL_DLL *BSBCreate( const char *pszFilename, int nCreationFlags, 
                            int nVersion, int nXSize, int nYSize );
int CPL_DLL BSBWritePCT( BSBInfo *psInfo, 
                         int nPCTSize, unsigned char *pabyPCT );
int CPL_DLL BSBWriteScanline( BSBInfo *psInfo, 
                              unsigned char *pabyScanlineBuf );
                            
CPL_C_END


#endif /* ndef _BSBREAD_H_INCLUDED */
