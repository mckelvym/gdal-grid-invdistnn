/******************************************************************************
 * $Id$
 *
 * Project:  GDAL
 * Purpose:  Prototypes for all format specific driver initializations.
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
 * Revision 1.6  2001/07/05 23:53:53  nemec
 * Add FIT file format
 *
 * Revision 1.5  2001/05/15 13:20:32  warmerda
 * added fujibas
 *
 * Revision 1.4  2001/04/02 17:12:01  warmerda
 * Added ecw support.
 *
 * Revision 1.3  2001/03/12 15:17:03  warmerda
 * added aaigrid
 *
 * Revision 1.2  2001/03/06 03:53:44  sperkins
 * Added FITS format support.
 *
 * Revision 1.1  2001/02/06 16:30:05  warmerda
 * New
 *
 */

#ifndef GDAL_FRMTS_H_INCLUDED
#define GDAL_FRMTS_H_INCLUDED

#include "cpl_port.h"

CPL_C_START
void CPL_DLL GDALRegister_GDB(void);
void CPL_DLL GDALRegister_GTiff(void);
void CPL_DLL GDALRegister_GXF(void);
void CPL_DLL GDALRegister_OGDI(void);
void CPL_DLL GDALRegister_HFA(void);
void CPL_DLL GDALRegister_AAIGrid(void);
void CPL_DLL GDALRegister_AIGrid(void);
void CPL_DLL GDALRegister_AIGrid2(void);
void CPL_DLL GDALRegister_CEOS(void);
void CPL_DLL GDALRegister_SAR_CEOS(void);
void CPL_DLL GDALRegister_SDTS(void);
void CPL_DLL GDALRegister_ELAS(void);
void CPL_DLL GDALRegister_EHdr(void);
void CPL_DLL GDALRegister_PAux(void);
void CPL_DLL GDALRegister_EFF(void);
void CPL_DLL GDALRegister_DOQ1(void);
void CPL_DLL GDALRegister_DTED(void);
void CPL_DLL GDALRegister_MFF(void);
void CPL_DLL GDALRegister_HKV(void);
void CPL_DLL GDALRegister_PNG(void);
void CPL_DLL GDALRegister_JPEG(void);
void CPL_DLL GDALRegister_MEM(void);
void CPL_DLL GDALRegister_JDEM(void);
void CPL_DLL GDALRegister_GRASS(void);
void CPL_DLL GDALRegister_PNM(void);
void CPL_DLL GDALRegister_GIF(void);
void CPL_DLL GDALRegister_Envisat(void);
void CPL_DLL GDALRegister_FITS(void);
void CPL_DLL GDALRegister_ECW(void);
void CPL_DLL GDALRegister_FujiBAS(void);
void CPL_DLL GDALRegister_FIT(void);
CPL_C_END

#endif /* ndef GDAL_FRMTS_H_INCLUDED */
